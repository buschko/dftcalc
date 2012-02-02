namespace DFT {
class DFTree;
}

#ifndef DFT_H
#define DFT_H

#include <algorithm>
#include <vector>
#include <map>
#include <iostream>
#include <assert.h>
#include "dftnodes/nodes.h"

class Parser;

namespace DFT {

/**
 * Dynamic Fault Tree. Contains all the data about a DFT.
 * To keep a DFTree consistent, make sure all Node object referenced by 
 * Nodes in a DFTree are added to the DFTree object using addNode().
 */
class DFTree {
private:

	// The nodes of the DFT
	std::vector<Nodes::Node*> nodes;
	
	// The mapping from name (e.g. "A") to a Node
	std::map<std::string,DFT::Nodes::Node*> nodeTable;
	
	// The Top (root) Node of the DFT
	Nodes::Node* topNode;

	/**
	 * Sets the Top node without any checks
	 */
	int setTopNode_(Nodes::Node* node) {
		this->topNode = node;
		return 0;
	}

	DFTree(const DFTree& d) {
	}
	
	DFTree& operator=(const DFTree& other) {
		return *this;
	}

public:
	DFTree(): nodes(0), topNode(NULL) {

	}
	virtual ~DFTree() {
		for(int i=nodes.size(); i--;) {
			assert(nodes.at(i));
			delete nodes.at(i);
		}
	}

	// Claims ownership
	/**
	 * Adds the specified node to this DFT.
	 * NOTE: it claims ownership of the Node; do not delete
	 * @param node The Node to add to the DFT.
	 */
	void addNode(Nodes::Node* node) {
		nodes.push_back(node);
	}
	
	/**
	 * Removed the specified node from this DFT.
	 * All references to this node are removed as well.
	 * NOTE: the Node itself is freed from memory as well!
	 * @param node The Node to remove from the DFT.
	 */
	vector<DFT::Nodes::Node*>::iterator removeNode(Nodes::Node* node) {
		
		// Loop over all the nodes in the DFT
		for(DFT::Nodes::Node* n: nodes) {
			
			// If current the node is a gate...
			if(n->isGate()) {
				DFT::Nodes::Gate* gate = static_cast<DFT::Nodes::Gate*>(n);
				
				// Remove all its child-references to the node to be removed
				vector<DFT::Nodes::Node*>::iterator it = std::remove(gate->getChildren().begin(),gate->getChildren().end(),node);
				gate->getChildren().resize(it-gate->getChildren().begin());
			}
			
			// Remove all the parent-references of the current node to the node to be removed
			vector<DFT::Nodes::Node*>::iterator it = std::remove(n->getParents().begin(),n->getParents().end(),node);
			n->getParents().resize(it-n->getParents().begin());
		}
		
		// Remove the node from the DFT
		vector<DFT::Nodes::Node*>::iterator it = std::remove(nodes.begin(),nodes.end(),node);
		nodes.resize(it-nodes.begin());
		
		// Delete the node
		delete node;
		
		// Return the iterator to the next valid node (or end())
		return it;
	}

	/**
	 * Returns the Node associated with the specified name.
	 * Returns NULL if no such Node exists.
	 * @return The Node associated with the specified name.
	 */
	Nodes::Node* getNode(const std::string& name) {
		for(size_t i=0; i<nodes.size(); ++i) {
			if(nodes.at(i)->getName()==name) {
				return nodes.at(i);
			}
		}
		return NULL;
	}

	/**
	 * Returns the list of nodes.
	 * @return The list of nodes.
	 */
	std::vector<Nodes::Node*>& getNodes() {
		return nodes;
	}

	/**
	 * Sets the Top Node to the specified Node.
	 * The Node has to be already added to this DFT.
	 * @param The Top Node to set.
	 * @return false: success; true: error
	 */
	bool setTopNode(Nodes::Node* node) {
		// Check if the specified node is in this DFT
		for(size_t i=0; i<nodes.size(); ++i) {
			if(nodes.at(i)==node) {
				return setTopNode_(node);
			}
		}
		return true; // FIXME: ERROR HANDLING
	}
	
	/**
	 * Returns the Top Node.
	 * @return The Top Node.
	 */
	Nodes::Node* getTopNode() {
		return topNode;
	}
	
	void transformFDEPNodes() {
		size_t s_before = nodes.size();
		size_t handled = 0;
		
		// Copy the current list of nodes
		const auto nodes = this->nodes;
		
		// Loop over all the nodes in the DFT
		for(DFT::Nodes::Node* node: nodes) {
			++handled;
			
			// If we find an FDEP Gate...
			if(DFT::Nodes::Node::typeMatch(node->getType(),DFT::Nodes::GateFDEPType)) {
				DFT::Nodes::GateFDEP* fdep = static_cast<DFT::Nodes::GateFDEP*>(node);
				
				// ... loop over all its dependers (the nodes that will fail if the FDEP fails)
				for(DFT::Nodes::Node* fdepChild: fdep->getDependers()) {
					assert(fdepChild && "fdepChild should not be NULL");
					
					// We will now build an OR node for this depender of the FDEP gate.
					
					// Set a name based on the depender
					std::string fdeporName = "FDEPOR_" + fdepChild->getName();
					
					DFT::Nodes::GateOr* gate = NULL;
					
					// Check if there already is an OR originating from the FDEP
					{
						DFT::Nodes::Node* gaten = getNode(fdeporName);
						if(gaten) {
							if(DFT::Nodes::Node::typeMatch(gaten->getType(),DFT::Nodes::GateOrType)) {
								gate = static_cast<DFT::Nodes::GateOr*>(gaten);
							} else {
								assert(0 && "For some reason one of the FDEPOR_ gates is not an OR");
							}
						}
						
					}
					
					// If there was not already an OR originating from the FDEP gate...
					if(!gate) {
						
						// ... create a new OR node
						gate = new DFT::Nodes::GateOr(node->getLocation(), fdeporName);
						
						// Add it to the DFT
						addNode(gate);
						
						// Create the child (depender) --> parent (OR-node) relationship
						fdepChild->getParents().push_back(gate);

						// Create the child (depender) <-- parent (OR-node) relationship
						gate->addChild(fdepChild);
						
						// Loop over all the parents of the depender
						for(DFT::Nodes::Node* parent: fdepChild->getParents()) {
							
							// Add the parent to the OR-node's parents
							gate->getParents().push_back(parent);
							assert(DFT::Nodes::Node::typeMatch(parent->getType(),DFT::Nodes::GateType) && "A parent should be a gate...");
							
							// Add the OR-node to the list of children of the parent
							DFT::Nodes::Gate* parentGate = static_cast<DFT::Nodes::Gate*>(parent);
							parentGate->getChildren().push_back(gate);
						}
					}
					
					// At this point we have created a new OR-node or are using an existing one.
					// We only need to add the source (trigger) of the FDEP to the mix.
					
					// Create the child (source) --> parent (OR-node) relationship
					fdep->getEventSource()->getParents().push_back(gate);

					// Create the child (source) <-- parent (OR-node) relationship
					gate->addChild(fdep->getEventSource());
				}

				// Remove the original FDEP node
				removeNode(node);
			}
		}
		assert(handled==s_before);
	}
};
} // Namespace: DFT

#endif // DFT_H
