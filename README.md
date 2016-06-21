# 2DSearchTree
This repo contains the source for a generic 2D search tree written in C++. A Usage example is also provided.

## Example

The example provided uses the tree to search for a set of sprites near a test sprite controlled by the mouse. A video of this example can be found [here](https://www.youtube.com/watch?v=62l-GwzC8qk).

## Usage

The user must implement the interface below that defines the behavior of the tree. `Value` is the type stored in the tree and `NodeCompare` defines a Node's search space.
```c++
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
	virtual bool satisfies(const NodeCompare& nodeCompare, Value val) = 0;

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
```