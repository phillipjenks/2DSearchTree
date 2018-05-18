/*

	- Generic 2D search tree
	Written by Phillip Jenks 2016

	Usage:
	Provide a predicate that implements the SearchPredicate<Value, NodeCompare> interface
	where Value is the value type of objects inserted into the tree and NodeCompare is
	the type that defines the search space for a node. i.e. Value could be a type with
	a defined vector position and NodeCompare could be a Rect

	The search space for each node is divided into four quadrants. A value can belong to
	more than one quadrant.
*/

#ifndef __SEARCH_TREE_2D_H_
#define __SEARCH_TREE_2D_H_

#include <vector>
#include <set>
#include <map>
#include <utility>
#include <memory>

// Utility enum to mark each search quadrant
// The values are chosen to allow bitwise operations
// since values can belong to more than one quadrant
enum class RegionCode {
	UPPER_LEFT = 1 << 0,
	UPPER_RIGHT = 1 << 1,
	LOWER_LEFT = 1 << 2,
	LOWER_RIGHT = 1 << 3
};

const std::size_t g_minDataSize = 3;

//=======================================
// Implementation interface
//=======================================
template<class Value, class NodeCompare>
class SearchPredicate {
public:
	// Returns default value for the node comparison type
	virtual NodeCompare nilCompare() = 0;

	// Used for the root node. Builds the root search space from a set of values
	// belonging to the tree
	// inputs:
	//		values - set of values to belonging to the tree
	// outputs:
	//		Search space used as the root search space for the tree
	virtual NodeCompare buildRegionFromData(const std::set<Value>& values) = 0;

	// Subdivides the search space of a parent into quadrants given a set of values belonging
	// to the parent
	// inputs: 
	//		parentRegion - search space for the parent node
	//		values - values belonging to the parent
	//		quads - A mapping of Region code to child search spaces. The NodeCompare values will be used to build the child nodes
	virtual void buildQuadrantsFromData(const NodeCompare& parentRegion, const std::set<Value>& values, const std::map<RegionCode, NodeCompare&>& quads) = 0;

	// Returns whether or not a value belongs to a node's search space
	// inputs:
	//		nodeCompare - a 2D search space
	//		val - Value to test against the search space
	// outputs:
	//		returns true if the value belongs to the search space
	virtual bool satisfies(const NodeCompare& nodeCompare, const Value& val) = 0;

	// Returns whether or not two search spaces overlap
	// Used to return all values that belong to a test search space
	// i.e. given a Rect, used to find all values that belong to nodes whose search space
	// overlaps the test Rect
	// inputs:
	//		compareLeft, compareRight - Two search spaces to test against each other
	// outputs:
	//		returns true if the search spaces overlap. false otherwise
	virtual bool overlaps(const NodeCompare& compareLeft, const NodeCompare& compareRight) = 0;
};

//=======================================
// Main Tree Interface
//=======================================
template<class Value, class NodeCompare, class Predicate>
class SearchTree2D {
public:

	using SetValue = std::set<Value>;

	// Default constructor
	SearchTree2D() = default;

	// Destructor
	~SearchTree2D();

	// Copy constructor
	SearchTree2D(const SearchTree2D&) = default;

	// Move Constructor
	SearchTree2D(SearchTree2D&&);

	// Assignment. Pass by value handles assignments by both lvalues and rvalues
	SearchTree2D& operator=(SearchTree2D);

	// swap operation
	friend void swap(SearchTree2D& left, SearchTree2D& right) {
		using std::swap;
		swap(left.m_tree, right.m_tree);
	}

	// Inserts a value into the tree
	// This may leave the tree unbalanced
	void add(const Value& val);

	// Removes a value from the tree
	void remove(const Value& val);

	// Empties the tree
	void clear();

	// Returns all values belonging to nodes whose search spaces overlap (as defined by the predicate)
	// with the input search space
	SetValue getNearbyValues(const NodeCompare& compare) const;

	// Rebalances the tree, possibly removing or adding nodes as necessary.
	// This should be called if the location of values in the tree may have changed
	// as the tree will not update on value changes
	void rebalance();

private:

	// Private Node class used for nodes in the tree
	class Node {
	public:

		// Constructor
		Node();

		// Destructor
		~Node();

		// Copy constructor
		Node(const Node&);

		// Assignments and Move constructor
		// Node is an internal class so we don't expect the client to need
		// these functions. Internally, we only use the copy constructor when
		// copying trees.
		Node(Node&&) = delete;
		Node& operator=(Node) = delete;
		Node& operator=(Node&&) = delete;

		// Adds value to the node
		void add(const Value& val);

		// Removes value from the node
		void remove(const Value& val);

		// clears the node
		void clear();

		// Returns all values belonging to nodes whose search spaces overlap (as defined by the predicate)
		// with the input search space
		SetValue getNearbyValues(const NodeCompare& compare) const;

		// Uses this node's data to build the search space as defined
		// by the predicate for the root node.
		void buildRootRegion();

		// Rebalances the tree, creating and deleting nodes as necessary
		void rebalance();

	private:

		using RegionMap = std::map<RegionCode, std::unique_ptr<Node> >;
		using QuadMap = std::map<RegionCode, NodeCompare&>;
		using QuadPair = std::pair<RegionCode, NodeCompare&>;

		// node's search space
		NodeCompare m_compare;

		// map holding child nodes keyed by region
		RegionMap m_mapRegions;

		// data belonging to this node (should be empty if this node has children)
		SetValue m_data;

		// Returns true if this node has children
		bool hasChildren() const;

		// Returns all values belonging to this node and its children
		SetValue getAllChildValues() const;

		// Deletes child nodes
		void deleteChildren();

		// Returns false if this node should be a leaf in the tree
		bool shouldSubdivide(const SetValue& values, const QuadMap& quads) const;

		// Sets the search space for this node
		void setCompare(const NodeCompare& compare);
	};

	Node m_tree;
};

// =========================================================
// Main Tree Implementation
// =========================================================
// Destructor
template<class Value, class NodeCompare, class Predicate>
SearchTree2D<Value, NodeCompare, Predicate>::~SearchTree2D() {

	clear();
}

// Move constructor
template<class Value, class NodeCompare, class Predicate>
SearchTree2D<Value, NodeCompare, Predicate>::SearchTree2D(SearchTree2D&& otherTree)
	: SearchTree2D()
{
	swap(*this, otherTree);
}

// Assignment operator. Passing other by value handles both lvalue and rvalue references
// lvalues will be copy contructed and rvalues will be move constructed
template<class Value, class NodeCompare, class Predicate>
SearchTree2D<Value, NodeCompare, Predicate>& SearchTree2D<Value, NodeCompare, Predicate>::operator=(SearchTree2D<Value, NodeCompare, Predicate> other) {
	swap(*this, other);
	return *this;
}

// Add a value to the tree
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::add(const Value& val) {

	m_tree.add(val);
}

// Remove a value from the tree
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::remove(const Value& val) {

	m_tree.remove(val);
}

// Clear the tree of all values
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::clear() {

	m_tree.clear();
}

// Get values belonging to leafs whose search space satisfies the test compare
template<class Value, class NodeCompare, class Predicate>
auto SearchTree2D<Value, NodeCompare, Predicate>::getNearbyValues(const NodeCompare& compare) const -> SetValue {

	return m_tree.getNearbyValues(compare);
}

// Rebalance our tree
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::rebalance() {

	// Build the root search space for our tree
	m_tree.buildRootRegion();

	// Rebalance the tree for the new search space
	m_tree.rebalance();
}

// =========================================================
// Node Implementation
// =========================================================
// Default Constructor
template<class Value, class NodeCompare, class Predicate>
SearchTree2D<Value, NodeCompare, Predicate>::Node::Node()
	: m_compare()
	, m_mapRegions()
	, m_data()
{
	Predicate predicate;
	m_compare = predicate.nilCompare();

	// build our child node mapping
	m_mapRegions[RegionCode::UPPER_LEFT] = nullptr;
	m_mapRegions[RegionCode::UPPER_RIGHT] = nullptr;
	m_mapRegions[RegionCode::LOWER_LEFT] = nullptr;
	m_mapRegions[RegionCode::LOWER_RIGHT] = nullptr;
}

// Destructor
template<class Value, class NodeCompare, class Predicate>
SearchTree2D<Value, NodeCompare, Predicate>::Node::~Node() {

	clear();
}

// Copy constructor
template<class Value, class NodeCompare, class Predicate>
SearchTree2D<Value, NodeCompare, Predicate>::Node::Node(const Node& other)
	: m_compare(other.m_compare)
	, m_mapRegions()
	, m_data(other.m_data)
{
	// build our child node mapping
	m_mapRegions[RegionCode::UPPER_LEFT] = nullptr;
	m_mapRegions[RegionCode::UPPER_RIGHT] = nullptr;
	m_mapRegions[RegionCode::LOWER_LEFT] = nullptr;
	m_mapRegions[RegionCode::LOWER_RIGHT] = nullptr;

	for (auto&& region : other.m_mapRegions) {
		if (region.second) {
			m_mapRegions[region.first] = std::unique_ptr<Node>(new Node(*(region.second)));
		}
	}
}

// Add a value to the node
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::Node::add(const Value& val) {

	Predicate predicate;

	if (hasChildren()) {
		bool wasAdded = false;
		for (auto&& region : m_mapRegions) {
			// Check children of they should hold the value
			if (region.second && predicate.satisfies(region.second->m_compare, val)) {
				region.second->add(val);
				wasAdded = true;
			}
		}

		if (!wasAdded) {
			// The new value wasn't added to any children. This means that there is
			// either a bug in predicate implementation or this is the root node
			// and the new value belongs outside of the root search space.
			// Either way, let's hold onto this value as part of this node and let a future
			// rebalance ensure the child search spaces satisfy this value
			m_data.insert(val);
		}
	}
	else {
		m_data.insert(val);
	}
}

// Remove a value from the node or its children
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::Node::remove(const Value& val) {

	if (hasChildren()) {
		for (auto&& region : m_mapRegions) {
			if (region.second) {
				region.second->remove(val);
			}
		}
	}

	m_data.erase(val);
}

// Clear the node and its children of all values
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::Node::clear() {
	if (hasChildren()) {
		for (auto&& region : m_mapRegions) {
			if (region.second) {
				region.second->clear();
			}
		}
		deleteChildren();
	}

	m_data.clear();
}

// Get values belonging to child leafs whos search space satisfies the test compare
template<class Value, class NodeCompare, class Predicate>
auto SearchTree2D<Value, NodeCompare, Predicate>::Node::getNearbyValues(const NodeCompare& compare) const -> SetValue {

	// Our return set
	SetValue nearbyVals;
	if (hasChildren()) {
		// Check children and get their values if compare overlaps with the childs's search space
		for (auto&& region : m_mapRegions) {
			if (region.second) {
				SetValue childVals = region.second->getNearbyValues(compare);
				nearbyVals.insert(childVals.begin(), childVals.end());
			}
		}
	}

	Predicate predicate;

	// Return our values if compare overlaps with our search space
	// This will also return orphaned values that belong to this node but not its children
	if (predicate.overlaps(m_compare, compare)) {

		// std::set guarantees uniqueness (values may belong to more than one node)
		nearbyVals.insert(m_data.begin(), m_data.end());
	}

	return nearbyVals;
}

// Build a root search space based off of current data
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::Node::buildRootRegion() {

	Predicate predicate;

	// Build our search space based off of our data
	m_compare = predicate.buildRegionFromData(getAllChildValues());
}

// Rebalance this node and its children
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::Node::rebalance() {

	Predicate predicate;

	SetValue setAllData = getAllChildValues();

	// Remove data that no longer satisfies this node's compare
	for (SetValue::iterator itSet = setAllData.begin(); itSet != setAllData.end(); ) {
		if (!predicate.satisfies(m_compare, *itSet)) {
			setAllData.erase(itSet++);
		}
		else {
			++itSet;
		}
	}

	// Clear our local set. This set will be reset if necessary and will also
	// hold onto orphaned values if necessary
	m_data.clear();

	if (hasChildren()) {
		// Should we keep the children?
		// first just check raw data size

		if (setAllData.size() <= g_minDataSize) {
			// Our data set is small enough that we don't need children for our search space
			deleteChildren();
			m_data = setAllData;
		}
		else {

			// Now check if subdividing will do anything

			// Let's update our child quadrants
			// First grab references to our children's search spaces
			QuadMap mapQuads;
			for (auto&& region : m_mapRegions) {
				mapQuads.insert(QuadPair(region.first, region.second->m_compare));
			}

			// Use our predicate to rebuild our quadrant search spaces
			predicate.buildQuadrantsFromData(m_compare, setAllData, mapQuads);

			// Do we still need children?
			if (shouldSubdivide(setAllData, mapQuads)) {

				// Re-add our data to our children
				for (auto thisVal : setAllData) {
					// This may modify m_data of the value is orphaned
					add(thisVal);
				}

				// rebalance our child nodes
				for (auto&& region : m_mapRegions) {
					if (region.second) {
						region.second->rebalance();
					}
				}
			}
			else {
				// We no longer need children. Just hold onto the data ourselves
				deleteChildren();
				m_data = setAllData;
			}
		}
	}
	// Should we subdivide and create children?
	else {
		// check raw data size
		if (setAllData.size() <= g_minDataSize) {
			// The data set is small enough that we don't need to create children
			m_data = setAllData;
		}
		else {

			// Let's build some test quads and see if they will subdivide

			NodeCompare ulComp = predicate.nilCompare();
			NodeCompare urComp = predicate.nilCompare();
			NodeCompare llComp = predicate.nilCompare();
			NodeCompare lrComp = predicate.nilCompare();

			QuadMap mapQuads;
			mapQuads.insert(QuadPair(RegionCode::UPPER_LEFT, ulComp));
			mapQuads.insert(QuadPair(RegionCode::UPPER_RIGHT, urComp));
			mapQuads.insert(QuadPair(RegionCode::LOWER_LEFT, llComp));
			mapQuads.insert(QuadPair(RegionCode::LOWER_RIGHT, lrComp));

			// Build our test quads from our data
			predicate.buildQuadrantsFromData(m_compare, setAllData, mapQuads);

			// Do we need children?
			if (shouldSubdivide(setAllData, mapQuads)) {

				// We need children, so build some child nodes and set their search spaces
				for (auto& region : m_mapRegions) {
					if (!region.second) {
						region.second = std::unique_ptr<Node>(new Node());
					}
					region.second->setCompare(mapQuads.at(region.first));
				}

				// Add the values to our children
				for (auto&& thisVal : setAllData) {
					// This may modify m_data of the value is orphaned
					add(thisVal);
				}

				// Rebalance our newly created children so they may create
				// children of their own
				for (auto&& region : m_mapRegions) {
					if (region.second) {
						region.second->rebalance();
					}
				}
			}
			else {

				// We don't need children
				m_data = setAllData;
			}
		}
	}
}

// Test if this node has children
template<class Value, class NodeCompare, class Predicate>
bool SearchTree2D<Value, NodeCompare, Predicate>::Node::hasChildren() const {

	// Check if we have at least one child
	bool hasChild = false;
	for (auto&& region : m_mapRegions) {
		if (region.second) {
			hasChild = true;
			break;
		}
	}
	return hasChild;
}

// Get all values belonging to this node and its children
template<class Value, class NodeCompare, class Predicate>
auto SearchTree2D<Value, NodeCompare, Predicate>::Node::getAllChildValues() const -> SetValue {

	// Gather all data belonging to this search space
	SetValue setData;
	if (hasChildren()) {
		for (auto&& region : m_mapRegions) {
			if (region.second) {
				SetValue childNodes = region.second->getAllChildValues();
				setData.insert(childNodes.begin(), childNodes.end());
			}
		}
	}

	// This will include orphaned values if we have them
	setData.insert(m_data.begin(), m_data.end());

	return setData;
}

// Delete children
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::Node::deleteChildren() {
	for (auto& region : m_mapRegions) {
		if (region.second) {
			region.second.reset(nullptr);
		}
	}
}

// Test whether or not this node needs to create children
template<class Value, class NodeCompare, class Predicate>
bool SearchTree2D<Value, NodeCompare, Predicate>::Node::shouldSubdivide(
	const SearchTree2D<Value, NodeCompare, Predicate>::SetValue& vecVals, 
	const QuadMap& mapQuads) const
{

	Predicate predicate; 

	// Is there a value that doesn't satisfy all regions?
	// If not, then all children will have the same values, so there is no need to subdivide
	// This is an admittedly simple test, but it works for an initial implementation
	// We could instead test if the tree would be well-balanced or we could 
	// limit by the number of operations
	bool inAllRegions = true;
	for (auto&& val : vecVals) {
		for (auto&& quad : mapQuads) {
			if (!predicate.satisfies(quad.second, val)) {
				inAllRegions = false;
				break;
			}
		}
		if (!inAllRegions) {
			break;
		}
	}

	return !inAllRegions;
}

// Set the search space for this node
template<class Value, class NodeCompare, class Predicate>
void SearchTree2D<Value, NodeCompare, Predicate>::Node::setCompare(const NodeCompare& compare) {
	m_compare = compare;
}

#endif