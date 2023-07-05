/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODE_HDR
#define QTPFS_NODE_HDR

#include <array>
#include <cinttypes>
#include <fstream>
#include <limits>
#include <variant>
#include <vector>


#include "PathEnums.h"
#include "PathDefines.h"

#include "System/float3.h"
#include "System/Rectangle.h"

#ifndef QTPFS_VIRTUAL_NODE_FUNCTIONS
#define QTNode INode
#endif

#define QTNODE_CHILD_COUNT 4

namespace QTPFS {
	struct NodeLayer;
	struct SearchNode;
	struct UpdateThreadData;

	struct INode {
			friend SearchNode;
	public:
		void SetNodeNumber(unsigned int n) { nodeNumber = n; }
		unsigned int GetNodeNumber() const { return nodeNumber; }

		unsigned int GetNeighborRelation(const INode* ngb) const;
		unsigned int GetRectangleRelation(const SRectangle& r) const;
		float GetDistance(const INode* n, unsigned int type) const;
		float2 GetNeighborEdgeTransitionPoint(const INode* ngb, const float3& pos, float alpha) const;
		SRectangle ClipRectangle(const SRectangle& r) const;

		unsigned int GetIndex() const { return index; }

		~QTNode() = default;
		QTNode() = default;
		QTNode(const QTNode& n) = delete;
		QTNode(QTNode&& n) = default;
		// QTNode(QTNode&& n) {
		// 	*this = std::move(n);
		// }

		QTNode& operator = (const QTNode& n) = delete;
		QTNode& operator = (QTNode&& n) = default;
		// QTNode& operator = (QTNode&& n) {
		// 	this->nodeNumber = n.nodeNumber;
		// 	this->index = n.index;
		// 	this->_xmax = n._xmax;
		// 	this->_xmin = n._xmin;
		// 	this->_zmax = n._zmax;
		// 	this->_zmin = n._zmin;
			
		// 	this->neighbours = std::move(n.neighbours);
		// 	this->childBaseIndex = n.childBaseIndex;
		// 	// if (this->childBaseIndex == -1u) {
		// 	// 	this->moveCostAvg = n.moveCostAvg;
		// 	// 	this->netpoints = std::move(n.netpoints);
		// 	// } else {
		// 		this->referenceNodeIndex = n.referenceNodeIndex;
		// 		this->moveCosts = std::move(n.moveCosts);
		// 	// }

		// 	return *this;
		// }

		static void InitStatic();

		void Init(
			const QTNode* parent,
			unsigned int nn,
			unsigned int x1, unsigned int z1,
			unsigned int x2, unsigned int z2,
			unsigned int idx
		);

		// NOTE:
		//     root-node identifier is always 0
		//     <i> is a NODE_IDX index in [0, 3]
		unsigned int GetChildID(unsigned int i, uint32_t rootMask) const {
			uint32_t rootId = rootMask & nodeNumber;
			uint32_t nodeId = ((~rootMask) & nodeNumber);
			return rootId | ((nodeId << 2) + (i + 1));
		}
		// unsigned int GetParentID() const { return ((nodeNumber - 1) >> 2); }

		std::uint64_t GetMemFootPrint(const NodeLayer& nl) const;
		std::uint64_t GetCheckSum(const NodeLayer& nl) const;

		void PreTesselate(NodeLayer& nl, const SRectangle& r, SRectangle& ur, unsigned int depth, const UpdateThreadData* threadData);
		void Tesselate(NodeLayer& nl, const SRectangle& r, unsigned int depth, const UpdateThreadData* threadData);
		void Serialize(std::fstream& fStream, NodeLayer& nodeLayer, unsigned int* streamSize, unsigned int depth, bool readMode);

		bool IsLeaf() const { return (childBaseIndex == -1u); }
		bool CanSplit(unsigned int depth, bool forced) const;

		bool Split(NodeLayer& nl, unsigned int depth, bool forced);
		bool Merge(NodeLayer& nl);

		unsigned int GetMaxNumNeighbors() const;
		// unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&);
		unsigned int GetNeighbors(const std::vector<int>&, std::vector<int>&);
		// const std::vector<INode*>& GetNeighbors(/*const std::vector<INode*>&*/);
		// bool UpdateNeighborCache(const std::vector<INode*>& nodes, int nodeLayer);
		bool UpdateNeighborCache(NodeLayer& nodeLayer, UpdateThreadData& threadData);

		void SetNeighborEdgeTransitionPoint(unsigned int ngbIdx, const float2& point) { netpoints[ngbIdx] = point; }
		const float2& GetNeighborEdgeTransitionPoint(unsigned int ngbIdx) const { return netpoints[ngbIdx]; }

		unsigned int xmin() const { return _xmin /*(_xminxmax  & 0xFFFF)*/; }
		unsigned int zmin() const { return _zmin /*(_zminzmax  & 0xFFFF)*/; }
		unsigned int xmax() const { return _xmax /*(_xminxmax >>     16)*/; }
		unsigned int zmax() const { return _zmax /*(_zminzmax >>     16)*/; }
		unsigned int xmid() const { return ((xmin() + xmax()) >> 1); }
		unsigned int zmid() const { return ((zmin() + zmax()) >> 1); }
		unsigned int xsize() const { return (xmax() - xmin()); }
		unsigned int zsize() const { return (zmax() - zmin()); }
		unsigned int area() const { return (xsize() * zsize()); }
		unsigned int point(int i) const { return points[i]; }


		bool RectIsInside(const SRectangle& rect) const {
			return
				xmin() <= rect.x1 && zmin() <= rect.y1 &&
				xmax() >= rect.x2 && zmax() >= rect.y2;
		}

		bool RectIntersects(const SRectangle& rect) const {
			return  !( xmin() >= rect.x2
					|| xmax() <= rect.x1
					|| zmin() >= rect.z2
					|| zmax() <= rect.z1);
		}

		// true iff this node is fully open (partially open nodes have larger but non-infinite cost)
		bool AllSquaresAccessible() const { return (moveCostAvg < (QTPFS_CLOSED_NODE_COST / float(area()))); }
		bool AllSquaresImpassable() const { return (moveCostAvg == QTPFS_POSITIVE_INFINITY); }

		void SetMoveCost(float cost) { moveCostAvg = cost; }
		//void SetSearchState(unsigned int state) { searchState = state; }
		// void SetMagicNumber(unsigned int number) { currMagicNum = number; }

		float GetSpeedMod() const { return 1.0f / moveCostAvg /*speedModAvg*/; }
		float GetMoveCost() const { return moveCostAvg; }
		// unsigned int GetSearchState() const { return searchState; }
		// unsigned int GetMagicNumber() const { return currMagicNum; }
		unsigned int GetChildBaseIndex() const { return childBaseIndex; }

		static unsigned int MinSizeX() { return MIN_SIZE_X; }
		static unsigned int MinSizeZ() { return MIN_SIZE_Z; }

		// const std::vector<INode*>& GetNeighbors() const {
		const std::vector<int>& GetNeighbors() const {
			return neighbours;
		}
		const std::vector<int>& GetNeighbors() { return neighbours; }
		const std::vector<float2>& GetNetPoints() const {
			return netpoints;
		}

		void DeactivateNode() { _xmin = std::numeric_limits<decltype(_xmin)>::max(); }

		bool NodeDeactivated() const { return (_xmin == std::numeric_limits<decltype(_xmin)>::max()); }

	private:
		bool UpdateMoveCost(
			const UpdateThreadData* threadData,
			const NodeLayer& nl,
			const SRectangle& r,
			unsigned int& numNewBinSquares,
			unsigned int& numDifBinSquares,
			unsigned int& numClosedSquares,
			bool& wantSplit,
			bool& needSplit
		);

		static unsigned int MIN_SIZE_X;
		static unsigned int MIN_SIZE_Z;

	public:
		static unsigned int MAX_DEPTH;

	private:
		unsigned int nodeNumber = -1u;
		unsigned int index = 0;

		union {
			struct {
				unsigned short _xmin;
				unsigned short _xmax;
				unsigned short _zmin;
				unsigned short _zmax;
			};
			std::array<unsigned short, 4> points = {};
		};

		float moveCostAvg = -1.0f;

		unsigned int childBaseIndex = -1u;
		std::vector<int> neighbours;
		std::vector<float2> netpoints;

		/*
		 * Component Node 16 bytes + 4
		 * Component HierachalNode 4 bytes + 4
		 * Component LeafNode 52 bytes + 4
		 * Component CoarseLeafNode 52 bytes + 4
		 */
	};

	struct CoarseLeafNode {
		unsigned int referenceNodeIndex; // reference for moveCosts calcs
		std::vector<int> neighbours;
		std::vector<float> moveCosts;
	};

	struct NodeSearched {};

	struct SearchNode {

		SearchNode() {}

		SearchNode(INode& srcNode)
			: nodeNumber(srcNode.nodeNumber)
			, index(srcNode.index)
			, prevNode(nullptr)
			// , netpoints(srcNode.netpoints)
			{}

		SearchNode(INode* srcNode)
			: nodeNumber(srcNode->nodeNumber)
			, index(srcNode->index)
			, prevNode(nullptr)
			// , netpoints(srcNode->netpoints)
			{}

		SearchNode(const SearchNode& other) { operator=(other); }

		SearchNode& operator=(const SearchNode& other) {
			nodeNumber = other.nodeNumber;
			fCost = QTPFS_POSITIVE_INFINITY;
			gCost = QTPFS_POSITIVE_INFINITY;
			hCost = QTPFS_POSITIVE_INFINITY;
			index = other.index;
			prevNode = other.prevNode;
			searchState = other.searchState;
			return *this;
		}

		void SetNodeNumber(unsigned int n) { nodeNumber = n; }
		// void SetHeapIndex(unsigned int n) { heapIndex = n; }
		unsigned int GetNodeNumber() const { return nodeNumber; }
		// unsigned int GetHeapIndex() const { return heapIndex; }
		float GetHeapPriority() const { return GetPathCost(NODE_PATH_COST_F); }
		unsigned int GetSearchState() const { return searchState; }

		bool operator <  (const SearchNode* n) const { return (fCost <  n->fCost); }
		bool operator >  (const SearchNode* n) const { return (fCost >  n->fCost); }
		bool operator == (const SearchNode* n) const { return (fCost == n->fCost); }
		bool operator <= (const SearchNode* n) const { return (fCost <= n->fCost); }
		bool operator >= (const SearchNode* n) const { return (fCost >= n->fCost); }

		void SetPathCosts(float g, float h) { fCost = g + h; gCost = g; hCost = h; }
		const float* GetPathCosts() const { return &fCost; }
		float GetPathCost(unsigned int type) const;

		void SetPrevNode(SearchNode* n) { prevNode = n; }
		SearchNode* GetPrevNode() const { return prevNode; }

		unsigned int GetIndex() const { return index; }
		void SetSearchState(unsigned int state) { searchState = state; }

		void SetNeighborEdgeTransitionPoint(const float2& point) { selectedNetpoint = point; }
		const float2& GetNeighborEdgeTransitionPoint() const { return selectedNetpoint; }

		unsigned int nodeNumber = -1u;
		unsigned int index = 0;
		unsigned int searchState = 0;

		float fCost = QTPFS_POSITIVE_INFINITY;
		float gCost = QTPFS_POSITIVE_INFINITY;
		float hCost = QTPFS_POSITIVE_INFINITY;

		// points back to previous node in path
		SearchNode* prevNode = nullptr;

		float2 selectedNetpoint;
	};
}

#endif

