#ifndef _CPP_FIBONACCI_
#define _CPP_FIBONACCI_

/** \mainpage cppfibonacci: a C++ implementation of Fibonacci heap
 *
 * For API, see fibonacci_heap.
 *
 * For sample code, see @ref example.cpp.
 *
 * GitHub address: https://github.com/zasdfgbnm/cppfibonacci
 */

#include <functional>
#include <tuple>
#include <initializer_list>
#include <memory>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <tuple>

#undef min
#undef max

#include <algorithm> 

using namespace std;

template <typename K, typename T, typename Compare>
class fibonacci_whitebox;

/** \brief A C++ implementation of Fibonacci heap
 *
 * @param K the type for keys
 * @param T the type for data
 * @param Compare the class that define the order of keys, with default value the "<".
 */
template <typename K, typename T, typename Compare = std::less<K>>
class fibonacci_heap {

public:
	class node;

private:

	/** To allow user defined test class to access private members of this class,
	  * simply define the test class name as macro FIBONACCI_HEAP_TEST_FRIEND
	  */
	friend class fibonacci_whitebox<K, T, Compare>;

	class internal_structure;
	class internal_data;
	// useful short for types
	using ssp = std::shared_ptr<internal_structure>;
	using swp = std::weak_ptr<internal_structure>;
	using dsp = std::shared_ptr<internal_data>;
	using dwp = std::weak_ptr<internal_data>;

	/** \brief the internal class responsible for the structure in Fibonacci heap
	 * Structural information and data are stored to make it easier for std::shared_ptr
	 * to automatically clean up memory without destroying user's pointer to data.
	 */
	class internal_structure {
	public:
		bool childcut = false;
		size_t degree = 0;
		dsp data;
		ssp right_sibling;
		swp left_sibling;
		ssp child;
		swp parent;
		~internal_structure() {
			data->structure.reset();
			// cut loops inside child's sibling list so that std::shared_ptr can
			// automatically free unneeded memory
			if (child) child->right_sibling = nullptr;
		}
		static ssp make_single_tree_forest(dsp data) {
			ssp ret = std::make_shared<internal_structure>();
			ret->data = data;
			data->structure = ret;
			ret->right_sibling = ret;
			ret->left_sibling = ret;
			return ret;
		}
	};

	/** \brief the interal class used to store data in Fibonacci heap */
	class internal_data {
	public:
		internal_data(K key, const T &data) :key(key), data(data) {}
		internal_data(K key, T &&data) :key(key), data(data) {}
		internal_data(const internal_data &old) = default;
		swp structure;
		K key;
		T data;
	};

	/** \brief recursively duplicate nodes and create a new forest, including structure node and data node
	 *
	 * @param root the root node of the tree to be duplicated
	 *
	 * @param head used to denote what phase this function is doing. If head==nullptr,
	 * then this function has just started at a doubly linked list; if head!=nullptr,
	 * then this function is checking inside the doubly linked list and head is the first
	 * element of the doubly linked list.
	 *
	 * @param newhead used only in the case when head!=nullptr, then newhead is
	 * the pointer towards the duplicated node of head
	 *
	 * @return pointer to the node of the duplicated tree
	 */
	static ssp duplicate_nodes(std::shared_ptr<const internal_structure> root, std::shared_ptr<const internal_structure> head, ssp parent, ssp newhead) {
		if (root == head) return nullptr;
		ssp newroot = std::make_shared<internal_structure>(*root);
		if (head == nullptr) {
			head = root;
			newhead = newroot;
		}
		// setup new data
		dsp newroot_data = std::make_shared<internal_data>(*(root->data));
		newroot_data->structure = newroot;
		newroot->data = newroot_data;
		// setup new right_sibling
		newroot->right_sibling = duplicate_nodes(root->right_sibling, head, parent, newhead);
		if (newroot->right_sibling == nullptr)
			newroot->right_sibling = newhead;
		// setup new left_sibling
		newroot->right_sibling->left_sibling = newroot;
		// setup new child
		newroot->child = duplicate_nodes(root->child, nullptr, newroot, nullptr);
		// setup new parent
		newroot->parent = parent;
		if (newroot->child) newroot->child->parent = newroot;
		return newroot;
	}

	/** \brief Meld another forest to this Fibonacci heap.
	 *
	 * Note that the degree of the node that has "target" as its child list will
	 * not be updated by this method and must be manually updated if neccessary.
	 *
	 * @param target pointer to one root of the target sibling list, default is
	 * the min pointer
	 *
	 * @param node pointer to the any root of the forest that will come in
	 *
	 * @param update_parent whether to update the parent pointers of all roots
	 * that will come in. This takes O(m) time, where m is number of elements in
	 * the sibling list spedified by "node".
	 *
	 * @param find_min whether to find the minimum of the roots at sibling list
	 * specified by "node". This takes O(m) time, where m is number of elements in
	 * the sibling list spedified by "node".
	 *
	 * @param set_min whether to set target to point towards the min of the  merged
	 * sibling list. If this parameter is true but "find_min" is false, the "node"
	 * is assumed to already point to the minimum root in the sibling list specified
	 * by "node".
	 *
	 * @param reset_childcut whether to reset the childcut value to false of roots
	 * in sibling list specified by "node". This takes O(m) time, where m is number
	 * of elements in the sibling list spedified by "node".
	 *
	 * @param parent if the target is empty there is no way to infer parent pointer
	 * from target, in this case this parameter will be used. This parameter is
	 * automatically ignored if target is not empty. Default value is nullptr.
	 */
	static void meld(ssp &target, ssp node, bool update_parent, bool find_min, bool set_min, bool reset_childcut, ssp parent = nullptr) {
		if (!node) return;
		if (target) parent = target->parent.lock();
		// update parent and find the min element
		// if neither updating the parent nor the finding min, nor reseting childcut
		// is needed, avoid this O(m) loop
		if (update_parent || find_min || reset_childcut) {
			ssp oldhead = node;
			ssp p = oldhead;
			do {
				if (update_parent) p->parent = parent;
				if (reset_childcut) p->childcut = false;
				if (find_min && Compare()(p->data->key, node->data->key))
					node = p;
				p = p->right_sibling;
			} while (p != oldhead);
		}
		// merge sibling lists
		if (target) {
			target->right_sibling.swap(node->right_sibling);
			target->right_sibling->left_sibling.swap(node->right_sibling->left_sibling);
			if (set_min && Compare()(node->data->key, target->data->key))
				target = node;
		}
		else {
			target = node;
		}
	}

	/** \brief insert a data node */
	node insert(dsp datanode) {
		_size++;
		ssp p = internal_structure::make_single_tree_forest(datanode);
		meld(min, p, true, false, true, false);
		return node(datanode);
	}

	/** \brief Remove the subtree rooted at p.
	 * The degree and child pointer of p's parent and left_sibling and right_sibling
	 * of p's siblings will be updated for consistency. Besides, sibling pointers of
	 * p are also updated so that p itself is a forest of one tree. p's parent pointer
	 * and childcut are kept unchanged.
	 */
	void remove_tree(ssp p) {
		if (!p->parent.expired()) {
			ssp pp = p->parent.lock();
			pp->degree--;
			if (pp->degree == 0)
				pp->child = nullptr;
			else if (pp->child == p)
				pp->child = p->right_sibling;
		}
		ssp &r = p->right_sibling;
		ssp &lr = p->left_sibling.lock()->right_sibling;
		swp &l = p->left_sibling;
		swp &rl = p->right_sibling->left_sibling;
		lr.swap(r);
		rl.swap(l);
	}

	/** \brief cascading cut */
	void cascading_cut(ssp p) {
		if (p == nullptr) return;
		ssp pp = p->parent.lock();
		if (pp) {
			if (p->childcut) {
				remove_tree(p);
				meld(min, p, true, false, true, false);
				cascading_cut(pp);
			}
			else
				p->childcut = true;
		}
	}

	/** \brief calculate the max degree of nodes */
	size_t max_degree() const {
		return std::floor(std::log(_size) / std::log((std::sqrt(5.0) + 1.0) / 2.0));
	}

	ssp min;
	size_t _size = 0;

public:

	/** \brief Create an empty Fibonacci heap. */
	fibonacci_heap() = default;

	/** \brief Initialize a Fibonacci heap from list of key data pairs.
	 * @param list the list of key data pairs
	 */
	fibonacci_heap(std::initializer_list<std::tuple<K, T>> list) {
		for (auto &i : list)
			insert(std::get<0>(i), std::get<1>(i));
	}

	/** \brief the copy constructor.
	 *
	 * Shallow copy will mess up the data structure and therefore is not allowed.
	 * Whenever the user tries to make a copy of a fibonacci_heap object, a deep
	 * copy will be made.
	 *
	 * Also note that the node objects at old Fibonacci heap can not be used at
	 * copied Fibonacci heap.
	 *
	 * @param old the Fibonacci heap to be copied
	 */
	fibonacci_heap(const fibonacci_heap &old) :min(duplicate_nodes(old.min, nullptr, nullptr, nullptr)), _size(old._size) {}

	/** \brief the move constructor.
	 *
	 * Move all the data from old Fibonacci heap to new one. The node objects at
	 * old Fibonacci heap can be used at new Fibonacci heap.
	 *
	 * @param old the Fibonacci heap to move data from
	 */
	fibonacci_heap(fibonacci_heap &&old) :min(old.min), _size(old._size) {
		old.min = nullptr;
		old._size = 0;
	}

	~fibonacci_heap() {
		// cut loops inside the forest list so that std::shared_ptr can
		// automatically free unneeded memory
		if (min) min->right_sibling = nullptr;
	}

	/** \brief the assignment operator, using copy-and-swap idiom
	 *
	 * @param old the Fibonacci heap to be copied
	 *
	 * @return reference to this object
	 */
	fibonacci_heap& operator = (fibonacci_heap old) {
		std::swap(this->_size, old._size);
		this->min.swap(old.min);
		return *this;
	}

	/** \brief Reference to nodes in Fibonacci heap.
	 *
	 * Objects of node should be returned from methods of fibonacci_heap,
	 * and will keep valid throughout the whole lifetime of the Fibonacci heap.
	 * If the original Fibonacci heap is copied to a new heap, node objects of
	 * the original Fibonacci heap will not work on the new heap.
	 */
	class node {

		friend class fibonacci_heap;

		/** \brief pointer to interanl node */
		dsp internal;

		/** \brief create a node object from internal nodes
		 *
		 * This is a private constructor, so the users are not allowed to create a node object.
		 * @param internal pointer to internal node
		 */
		node(ssp internal) :internal(internal->data) {}

		/** \brief create a node object from internal nodes
		 *
		 * This is a private constructor, so the users are not allowed to create a node object.
		 * @param internal pointer to internal node
		 */
		node(dsp internal) :internal(internal) {}

	public:

		/** \brief this will create an empty node that don't belong to any Fibonacci heap */
		node() = default;

		/** \brief get the key of this node.
		 * @return the key of this node
		 */
		K key() const { return internal->key; }

		/** \brief get the data stored in this node.
		 * @return the lvalue holding the data stored in this node
		 */
		T &data() { return internal->data; }

		/** \brief get the data stored in this node.
		 * @return the rvalue holding the data stored in this node
		 */
		const T &data() const { return internal->data; }

		/** \brief operator to test if two node are the same */
		bool operator==(node rhs) {
			return internal == rhs.internal;
		}

	};

	/** \brief Return the number of elements stored.
	 *
	 * @return number of elements stored in this Fibonacci heap
	 */
	size_t size() const { return _size; }

	/** \brief Insert an element.
	 *
	 * @param key the key of the element to be inserted
	 * @param data the data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(K key, const T &data) { return insert(std::make_shared<internal_data>(key, data)); }

	/** \brief Insert an element.
	 *
	 * @param key the key of the element to be inserted
	 * @param data the data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(K key, T &&data) { return insert(std::make_shared<internal_data>(key, data)); }

	/** \brief Insert an element.
	 *
	 * Note that the node object passed as parameter will NOT be updated to point to the
	 * inserted node in Fibonacci heap. The user need to keep the return value in order
	 * to keep track of the newly inserted node.
	 *
	 * @param n the node object holding the key and data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(node n) { return insert(n.key(), n.data()); }

	/** \brief Return the top element.
	 * @return the node object on the top
	 */
	node top() const {
		if (_size == 0) throw "this Fibonacci heap is empty";
		return node(min);
	}

	/** \brief Meld another Fibonacci heap to this Fibonacci heap.
	 *
	 * After meld, all the data will be moved to this Fibonacci heap, and the
	 * parameter "fh" will become empty. After meld, both the node objects of
	 * this and the node objects of parameter "fh" will work on this.
	 *
	 * @param fh the Fibonacci heap to be melded
	 */
	void meld(fibonacci_heap<K, T, Compare> &fh) {
		meld(min, fh.min, false, false, true, false);
		fh.min = nullptr;
		_size += fh._size;
		fh._size = 0;
	}

	/** \brief Descrease (or increase if you use greater as Compare) the key of the given node.
	 *
	 * It is the user's responsibility to make sure that the given node is
	 * actually in this Fibonacci heap. Trying to decrease a key of a node
	 * not in this Fibonacci heap will have undefined behavior.
	 *
	 * @param n the node object holding the key and data of the element to be inserted
	 * @param new_key the new key of the node
	 */
	void decrease_key(node n, K new_key) {
		if (Compare()(n.key(), new_key)) throw "increase_key is not supported";
		if (n.internal->structure.expired()) throw "the given node is not in this Fibonacci heap";
		ssp ns = n.internal->structure.lock();
		ssp p = ns->parent.lock();
		n.internal->key = new_key;
		if (p) {
			if (Compare()(new_key, p->data->key)) {
				remove_tree(ns);
				meld(min, ns, true, false, true, false);
				cascading_cut(p);
			}
		}
		else if (Compare()(new_key, min->data->key))
			min = ns;
	}

	/** \brief Remove the top element.
	 * @return the removed node object
	 */
	node remove() {
		if (_size == 0) throw "no element to remove";
		ssp oldmin = min;
		if (_size == 1) {
			_size = 0;
			min = nullptr;
			return node(oldmin);
		}

		// merge trees of same degrees
		std::vector<ssp> trees(max_degree() + 1);
		if (min->child)
			meld(min, min->child, false, false, false, false);
		while (min->right_sibling != min) {
			ssp q = min->right_sibling;
			remove_tree(q);
			while (trees[q->degree]) {
				bool q_is_smaller = Compare()(q->data->key, trees[q->degree]->data->key);
				ssp smaller = q_is_smaller ? q : trees[q->degree];
				ssp larger = q_is_smaller ? trees[q->degree] : q;
				trees[q->degree] = nullptr;
				meld(smaller->child, larger, true, false, false, true, smaller);
				smaller->degree++;
				q = smaller;
			}
			trees[q->degree] = q;
		}

		// meld trees of different degree back
		// cut the loop so that the resource for deleted structure node can be
		// cleaned up by std::shared_ptr.
		min->right_sibling = nullptr;
		min = nullptr;
		for (ssp p : trees) {
			if (!p) continue;
			meld(min, p, true, false, true, false);
		}

		_size--;
		oldmin->data->structure.reset();
		return node(oldmin->data);
	}

	/** \brief Remove the element specified by the node object.
	 *
	 * It is the user's responsibility to make sure that the given node is
	 * actually in this Fibonacci heap. Trying to remove a node not in this
	 * Fibonacci heap will have undefined behavior.
	 *
	 * @param n the node to be removed
	 * @return the removed node object
	 */
	node remove(node n) {
		if (n.internal->structure.expired()) throw "the given node is not in this Fibonacci heap";
		ssp p = n.internal->structure.lock();
		if (p == min) return remove();
		_size--;
		// remove n from tree
		remove_tree(p);
		p->data->structure.reset();
		// insert n's child back
		if (p->child) meld(min, p->child, true, true, true, false);
		// cascading cut
		cascading_cut(p->parent.lock());
		// cut the loop so that the resource for deleted structure node can be
		// cleaned up by std::shared_ptr.
		p->child = nullptr;
		p->right_sibling = nullptr;
		return n;
	}

	/** \brief generate the graph in dot format which can be used for illustration
	 *
	 * @param node_format a function that given the pointer address, key and data
	 * of a node and returns the format string in [] for this node. Default is always
	 * returns "label=<key>", which means only the key will be displayed. Return a
	 * "style=invis" if you don't want to see nodes.
	 *
	 * @param child_format fortmat string in [] for child pointer, default
	 * "color=black", set it to "style=invis" if you don't want it to display.
	 *
	 * @param parent_format fortmat string in [] for parent pointer, default
	 * "color=green", set it to "style=invis" if you don't want it to display.
	 *
	 * @param right_sibling_format fortmat string in [] for right_sibling pointer,
	 * default "color=red", set it to "style=invis" if you don't want it to display.
	 *
	 * @param left_sibling_format fortmat string in [] for left_sibling pointer,
	 * default "color=blue", set it to "style=invis" if you don't want it to display.
	 *
	 * @param double_arrow_format to display correctly, the case that two arrows
	 * of different style has same starting and ending is a special case to handle.
	 * This happens when both left_sibling and right_sibling of a node point to itself,
	 * or when there are two elements in a sibling list. This parameter is the format
	 * string in [] for this special case. Usually you need to create a double direction
	 * arrow with colors the color for left_sibling and the color for right_sibling.
	 * Default value is "dir=both color="red:blue"".
	 *
	 * @return string containing the dot format of this Fibonacci heap
	 */
	std::string dot(std::string node_format(void *address, const K &key, const T &data) = [](void *, const K &key, const T &) { return "label=" + std::to_string(key); },
		std::string child_format = "color=black",
		std::string parent_format = "color=green",
		std::string right_sibling_format = "color=red",
		std::string left_sibling_format = "color=blue",
		std::string double_arrow_format = "dir=both color=\"red:blue\""
	) const {
		using nodes_t = std::map<int, std::vector<std::string>>;
		std::function<std::tuple<nodes_t, std::string>(int, ssp, ssp)> traverse = [&](int depth, ssp start, ssp end)->std::tuple<nodes_t, std::string> {
			if (!start) return std::make_tuple(nodes_t(), "");
			if (start == end) return std::make_tuple(nodes_t(), "");
			bool head_of_sibling_list = !end;
			if (head_of_sibling_list) end = start;

			nodes_t nodes;
			std::ostringstream oss_arrows;

			// insert this node to node map
			std::ostringstream oss_nodes;
			oss_nodes << "addr" << start;
			if (start->data)
				oss_nodes << "[" << node_format(start.get(), start->data->key, start->data->data) << "];";
			nodes[depth] = { oss_nodes.str() };

			// print pointers of start node
			if (start == start->right_sibling&&start == start->left_sibling.lock()) {
				oss_arrows << "addr" << start << "->addr" << start << "[" << double_arrow_format << "];";
			}
			else {
				if (start->right_sibling) {
					if (start->right_sibling->left_sibling.lock() == start)
						oss_arrows << "addr" << start << "->addr" << start->right_sibling << "[" << double_arrow_format << "];";
					else
						oss_arrows << "addr" << start << "->addr" << start->right_sibling << "[" << right_sibling_format << "];";
				}
				if (!start->left_sibling.expired() && start->left_sibling.lock()->right_sibling != start)
					oss_arrows << "addr" << start << "->addr" << start->left_sibling.lock() << "[" << left_sibling_format << "];";
			}
			if (start->child)
				oss_arrows << "addr" << start << "->addr" << start->child << "[" << child_format << "];";
			if (!start->parent.expired())
				oss_arrows << "addr" << start << "->addr" << start->parent.lock() << "[" << parent_format << "];";

			// collect and combine results from other elements in sibling lilst and children
			std::function<void(nodes_t &)> merge_nodes = [&](nodes_t &a) {
				for (auto i = a.begin(); i != a.end(); ++i) {
					int depth = i->first;
					std::vector<std::string> v = i->second;
					if (nodes.count(depth) > 0)
						nodes[depth].insert(nodes[depth].end(), v.begin(), v.end());
					else
						nodes[depth] = v;
				}
			};
			std::tuple<nodes_t, std::string> sibling_output = traverse(depth, start->right_sibling, end);
			merge_nodes(std::get<0>(sibling_output));
			oss_arrows << std::get<1>(sibling_output);
			std::tuple<nodes_t, std::string> child_output = traverse(depth + 1, start->child, nullptr);
			merge_nodes(std::get<0>(child_output));
			oss_arrows << std::get<1>(child_output);
			return std::make_tuple(nodes, oss_arrows.str());
		};
		std::ostringstream oss;
		oss << "digraph{min->addr" << min << "[" << child_format << "];";
		if (!min)
			oss << "addr0[style=invis];}";
		else {
			std::tuple<nodes_t, std::string> traverse_output = traverse(0, min, nullptr);
			nodes_t nodes = std::get<0>(traverse_output);
			// output nodes
			int i = 0;
			while (nodes.count(i) > 0) {
				oss << "{rank=same;";
				for (std::string &n : nodes[i])
					oss << n;
				oss << "};";
				i++;
			}
			// output edges
			oss << std::get<1>(traverse_output) << "}";
		}
		return oss.str();
	}
};

#endif