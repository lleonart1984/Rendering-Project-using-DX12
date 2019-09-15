#include "../../Common/CS_Constants.h"

#define NONE -1
// Build file
/*
* Structures
*/

struct PITNode {
    int Parent;
    int Children[2];
    float Discriminant;
};

struct Fragment {
    int Index;
    float2 Interval;
};

/*
* Input
*/
StructuredBuffer<Fragment> fragments		: register(t0); // A-Buffer
/*
* Output (reused buffers)
*/
RWStructuredBuffer<int>		firstBuffer		: register(u0); // first fragment of per-node linked list
RWStructuredBuffer<PITNode> nodeBuffer		: register(u1); // tree information used only during construction
RWStructuredBuffer<float4> boundaryBuffer	: register(u2); // per-node local and global boundaries
RWTexture2D<int>			rootBuffer		: register(u3); // first buffer updated to be tree root nodes...
RWStructuredBuffer<int>	nextBuffer			: register(u4); // next buffer updated to be links of each per-node lists
RWStructuredBuffer<int> malloc				: register(u5); // node malloc buffer
RWStructuredBuffer<int> depth               : register(u6); // per-node depth value (Only for debugging)

int AllocateNode(int parent, int idxFragment, float2 interval)
{
    int ptrNode;
    InterlockedAdd(malloc[0], 1, ptrNode); // allocate in node buffer

    nextBuffer[idxFragment] = NONE;

    // Connectivity info
    nodeBuffer[ptrNode].Parent = parent;
    nodeBuffer[ptrNode].Children[0] = NONE;
    nodeBuffer[ptrNode].Children[1] = NONE;

    // Discriminant (decision value)
    nodeBuffer[ptrNode].Discriminant = lerp(interval.x, interval.y, 0.5);

    // Setting boundaries of node with initial interval
    boundaryBuffer[ptrNode] = interval.xyxy;

    if (parent == NONE)
        depth[ptrNode] = 0;
    else
        depth[ptrNode] = depth[parent] + 1;

    return ptrNode;
}

[numthreads(CS_BLOCK_SIZE_2D, CS_BLOCK_SIZE_2D, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int currentFragmentIdx = rootBuffer[DTid.xy]; // rootBuffer is the first in the linked list at the beginning

    int ROOT = NONE; // start with empty tree

    while (currentFragmentIdx != -1)
    {
        int nextFragmentIdx = nextBuffer[currentFragmentIdx]; // save because it will be modified

        float3 interval = float3(fragments[currentFragmentIdx].Interval, 0);

        // Save the fragment in the tree
        int parentNode = NONE;
        int currentNode = ROOT; // start in root node

        bool lastMoveToRight = false;
        while (currentNode != NONE)
        {
            interval.z = nodeBuffer[currentNode].Discriminant;
            int2 test = interval.xz < interval.zy;

            if (all(test))
                break;

            lastMoveToRight = test.y;
            parentNode = currentNode;
            currentNode = nodeBuffer[currentNode].Children[lastMoveToRight];
        }

        if (currentNode == NONE) // needs to create a new node
        {
            currentNode = AllocateNode(parentNode, currentFragmentIdx, interval.xy);
            if (parentNode == NONE)
                ROOT = currentNode;
            else
                nodeBuffer[parentNode].Children[lastMoveToRight] = currentNode;
        }
        else
        {
            nextBuffer[currentFragmentIdx] = firstBuffer[currentNode];
            boundaryBuffer[currentNode] =
                float2(
                    min(boundaryBuffer[currentNode].x, interval.x),
                    max(boundaryBuffer[currentNode].y, interval.y)).xyxy; // update local and global boundary per node.
        }
        firstBuffer[currentNode] = currentFragmentIdx;

        currentFragmentIdx = nextFragmentIdx;
    }

    rootBuffer[DTid.xy] = ROOT;
}