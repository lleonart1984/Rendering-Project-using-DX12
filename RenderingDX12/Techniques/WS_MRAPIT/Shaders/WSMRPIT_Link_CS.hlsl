#include "../../Common/CS_Constants.h"

struct PITNode {
    int Parent;
    int Children[2];

    float Discriminant;
};

cbuffer ComputeShaderInfo : register(b0)
{
    uint3 InputSize;
}

StructuredBuffer<int> rootBuffer                : register(t0); // Allocated arrays starts

RWStructuredBuffer<PITNode> nodeBuffer          : register(u0); // Interval Tree (will be updated the Global info)
RWStructuredBuffer<float4> boundaryNodeBuffer   : register(u1);
RWStructuredBuffer<int> preorderBuffer          : register(u2); // Reference to next node in preorder
RWStructuredBuffer<int> skipBuffer              : register(u3); // Reference to next node in preorder skipping current subtree

void GoParent(inout int node, out int state)
{
    int parent = nodeBuffer[node].Parent;
    if (parent == -1)
    {
        state = 0;
    }
    else
    {
        boundaryNodeBuffer[parent].z = min(boundaryNodeBuffer[parent].z, boundaryNodeBuffer[node].z);
        boundaryNodeBuffer[parent].w = max(boundaryNodeBuffer[parent].w, boundaryNodeBuffer[node].w);
        state = 2 + (nodeBuffer[parent].Children[0] != node); // 2 or 3
    }

    node = parent;
}

void ResolveReferencesToNextNonChildrenInPreorder(int root)
{
    int currentNode = root;
    int lastAnalized = -1;
    int firstUp = -1;
    int firstDown = -1;

    int state = 0;

    skipBuffer[root] = -1;

    while (currentNode != -1)
    {
        [branch]
        if (state == 0)
        {
            // Updating preorder linkage
            if (lastAnalized != -1) {
                preorderBuffer[lastAnalized] = currentNode;
            }
            lastAnalized = currentNode;

            // Updating skipping linkage
            [branch]
            if (firstUp != -1) {
                while (firstUp != firstDown)
                {
                    skipBuffer[firstUp] = currentNode;
                    firstUp = nodeBuffer[firstUp].Parent;
                }

                firstUp = -1;
                firstDown = -1;
            }

            state = 1;
        }
        else
            [branch]
        if (state == 3)
        {
            [branch]
            if (firstUp == -1) {
                firstUp = currentNode;
                firstDown = -1;
            }

            GoParent(currentNode, state);
        }
        else
        {
            int childToGo = nodeBuffer[currentNode].Children[state != 1];// ? nodeBuffer[currentNode].LeftChild : nodeBuffer[currentNode].RightChild;

            [branch]
            if (childToGo != -1)
            {
                currentNode = childToGo;
                state = 0;
            }
            else
            {
                state++;
            }
        }
    }

    preorderBuffer[lastAnalized] = -1;

    while (firstUp != -1)
    {
        skipBuffer[firstUp] = -1;
        firstUp = nodeBuffer[firstUp].Parent;
    }
}

[numthreads(CS_GROUPSIZE_1D, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= InputSize.x) {
        return;
    }

    int root = rootBuffer[DTid.x];
    if (root != -1) {
        ResolveReferencesToNextNonChildrenInPreorder(root);
    }
}