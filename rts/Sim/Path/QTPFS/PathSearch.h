/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHSEARCH_HDR
#define QTPFS_PATHSEARCH_HDR

#include <queue>
#include <vector>

#include "PathDefines.h"
#include "PathThreads.h"

#include "System/float3.h"

namespace QTPFS {
	struct IPath;
	struct NodeLayer;
	struct PathCache;
	struct SearchNode;

	namespace PathSearchTrace {
		struct Iteration {
			Iteration() { nodeIndices.push_back(-1u); }
			Iteration(const Iteration& i) { *this = i; }
			Iteration(Iteration&& i) { *this = std::move(i); }

			Iteration& operator = (const Iteration& i) { nodeIndices = i.nodeIndices; return *this; }
			Iteration& operator = (Iteration&& i) { nodeIndices = std::move(i.nodeIndices); return *this; }

			void Clear() {
				nodeIndices.clear();
				nodeIndices.push_back(-1u);
			}
			void SetPoppedNodeIdx(unsigned int i) { (nodeIndices.front()) = i; }
			void AddPushedNodeIdx(unsigned int i) { (nodeIndices.push_back(i)); }

			const std::vector<unsigned int>& GetNodeIndices() const { return nodeIndices; }

		private:
			// NOTE: indices are only valid so long as tree is not re-tesselated
			std::vector<unsigned int> nodeIndices;
		};

		struct Execution {
			Execution(unsigned int f): searchFrame(f) {}
			Execution(const Execution& e) = delete;
			Execution(Execution&& e) { *this = std::move(e); }

			Execution& operator = (const Execution& e) = delete;
			Execution& operator = (Execution&& e) {
				searchFrame = e.GetFrame();
				iterations = std::move(e.iterations);
				return *this;
			}

			void AddIteration(const Iteration& iter) { iterations.push_back(iter); }
			const std::vector<Iteration>& GetIterations() const { return iterations; }

			unsigned int GetFrame() const { return searchFrame; }

			size_t GetMemFootPrint() const {
				return iterations.size() * sizeof(decltype(iterations)::value_type);
			};

		private:
			std::vector<Iteration> iterations;

			// sim-frame at which the search was executed
			unsigned int searchFrame;
		};
	}

	struct PathSearch {
		void SetID(unsigned int n) { searchID = n; }
		void SetTeam(unsigned int n) { searchTeam = n; }
		unsigned int GetID() const { return searchID; }
		unsigned int GetTeam() const { return searchTeam; }

	protected:
		unsigned int searchID;     // links us to the temp-path that this search will finalize
		unsigned int searchTeam;   // which team queued this search

		unsigned int searchType;   // indicates if Dijkstra (h==0) or A* (h!=0) search is employed
		unsigned int searchState;  // offset that identifies nodes as part of current search

	public:
		PathSearch()
			: searchID(0)
			, searchTeam(0)
			, searchType(0)
			, searchState(0)
			, nodeLayer(nullptr)
			, searchExec(nullptr)
			, hCostMult(0.0f)
			, haveFullPath(false)
			, havePartPath(false)

			{}
		PathSearch(unsigned int pathSearchType)
			: PathSearch()
			{ searchType = pathSearchType; }
		~PathSearch() { /* openNodes->reset(); */ }

		void Initialize(
			NodeLayer* layer,
			const float3& sourcePoint,
			const float3& targetPoint,
			const CSolidObject* owner
		);
		void InitializeThread(SearchThreadData* threadData);
		bool Execute(unsigned int searchStateOffset = 0);
		void Finalize(IPath* path);
		bool SharedFinalize(const IPath* srcPath, IPath* dstPath);
		PathSearchTrace::Execution* GetExecutionTrace() { return searchExec; }

		const std::uint64_t GetHash() const { return pathSearchHash; };

		bool PathWasFound() const { return haveFullPath | havePartPath; }

		void SetPathType(int newPathType) { pathType = newPathType; }
		int GetPathType() const { return pathType; }

	private:
		struct DirectionalSearchData {
			DirectionalSearchData()
				: openNodes(nullptr)
				, srcSearchNode(nullptr)
				, tgtSearchNode(nullptr)
			{}

			// global queue: allocated once, re-used by all searches without clear()'s
			// this relies on INode::operator< to sort the INode*'s by increasing f-cost
			SearchPriorityQueue* openNodes;

			SearchNode *srcSearchNode, *tgtSearchNode;
			float3 srcPoint, tgtPoint;
		};

		void ResetState(SearchNode* node, struct DirectionalSearchData& searchData);
		void UpdateNode(SearchNode* nextNode, SearchNode* prevNode, unsigned int netPointIdx);

		void IterateNodes(unsigned int searchDir);
		void IterateNodeNeighbors(const INode* curNode, unsigned int searchDir);

		void TracePath(IPath* path);
		void SmoothPath(IPath* path) const;
		bool SmoothPathIter(IPath* path) const;

		bool ExecutePathSearch();
		bool ExecuteRawSearch();

		const std::uint64_t GenerateHash(const INode* srcNode, const INode* tgtNode) const;
		const std::uint64_t GenerateHash2(uint32_t p1, uint32_t p2) const;

		const std::uint64_t GenerateVirtualHash(const INode* srcNode, const INode* tgtNode) const;
		const std::uint32_t GenerateVirtualNodeNumber(const INode* startNode, int x, int z) const;

		QTPFS::SearchThreadData* searchThreadData;

		std::uint64_t pathSearchHash;
		std::uint64_t pathPartialSearchHash;

		const CSolidObject* pathOwner;
		NodeLayer* nodeLayer;
		int pathType;

		// not used unless QTPFS_TRACE_PATH_SEARCHES is defined
		PathSearchTrace::Execution* searchExec;
		PathSearchTrace::Iteration searchIter;

		SearchNode *curSearchNode, *nextSearchNode;
		SearchNode *minSearchNode;

		DirectionalSearchData directionalSearchData[2];

		float2 netPoints[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];

		float gDists[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];
		float hDists[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];
		float gCosts[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];
		float hCosts[QTPFS_MAX_NETPOINTS_PER_NODE_EDGE];

		float hCostMult;

		bool haveFullPath;
		bool havePartPath;
		bool badGoal;

public:
		bool rawPathCheck;

		static constexpr std::uint64_t BAD_HASH = std::numeric_limits<std::uint64_t>::max();
	};
}

#endif

