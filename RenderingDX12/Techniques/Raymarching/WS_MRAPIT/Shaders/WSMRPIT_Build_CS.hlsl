#include "../../../Common/CS_Constants.h"
#define NONE -1

struct Fragment {
    int Index;
    float2 Interval;
};

struct PITNode {
    int Parent;
    int Children[2];
    float Discriminant;
};

cbuffer ComputeShaderInfo : register(b0)
{
    uint3 InputSize;
}

StructuredBuffer<Fragment> fragments            : register(t0);

RWStructuredBuffer<int> rootBuffer              : register(u0);
RWStructuredBuffer<PITNode> nodeBuffer          : register(u1);
RWStructuredBuffer<float4> boundaryNodeBuffer   : register(u2);
RWStructuredBuffer<int> firstBuffer             : register(u3);
RWStructuredBuffer<int> nextBuffer              : register(u4);
RWStructuredBuffer<int> malloc                  : register(u5);

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
    boundaryNodeBuffer[ptrNode] = interval.xyxy;

    //if (parent == NONE)
    //    depth[ptrNode] = 0;
    //else
    //    depth[ptrNode] = depth[parent] + 1;

    return ptrNode;
}

[numthreads(CS_GROUPSIZE_1D, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= InputSize.x) {
        return;
    }

    int currentFragmentIdx = rootBuffer[DTid.x]; // rootBuffer is the first in the linked list at the beginning
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

            if (all(test)) {
                break;
            }

            lastMoveToRight = test.y;
            parentNode = currentNode;
            currentNode = nodeBuffer[currentNode].Children[lastMoveToRight];
        }

        if (currentNode == NONE) // needs to create a new node
        {
            currentNode = AllocateNode(parentNode, currentFragmentIdx, interval.xy);
            if (parentNode == NONE)
            {
                ROOT = currentNode;
            }
            else
            {
                nodeBuffer[parentNode].Children[lastMoveToRight] = currentNode;
            }
        }
        else
        {
            nextBuffer[currentFragmentIdx] = firstBuffer[currentNode];
            boundaryNodeBuffer[currentNode] =
                float2(
                    min(boundaryNodeBuffer[currentNode].x, interval.x),
                    max(boundaryNodeBuffer[currentNode].y, interval.y)).xyxy; // update local and global boundary per node.
        }

        firstBuffer[currentNode] = currentFragmentIdx;
        currentFragmentIdx = nextFragmentIdx;
    }

    rootBuffer[DTid.x] = ROOT;
}