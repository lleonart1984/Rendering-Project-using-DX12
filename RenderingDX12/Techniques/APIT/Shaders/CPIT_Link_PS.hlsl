struct PITNode {
    int Parent;
    int Children[2];

    float Discriminant;
};

Texture2D<int> RootBuffer					: register(t0); // Allocated arrays starts

RWStructuredBuffer<PITNode> NodeBuffer 		: register(u0); // Interval Tree (will be updated the Global info)
RWStructuredBuffer<float4> boundaryBuffer	: register(u1);
RWStructuredBuffer<int> PreorderBuffer		: register(u2); // Reference to next node in preorder
RWStructuredBuffer<int> SkipBuffer			: register(u3); // Reference to next node in preorder skipping current subtree

void GoParent(inout int node, out int state)
{
    int parent = NodeBuffer[node].Parent;
    if (parent == -1)
        state = 0;
    else
    {
        boundaryBuffer[parent].z = min(boundaryBuffer[parent].z, boundaryBuffer[node].z);
        boundaryBuffer[parent].w = max(boundaryBuffer[parent].w, boundaryBuffer[node].w);
        state = 2 + (NodeBuffer[parent].Children[0] != node); // 2 or 3
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

    SkipBuffer[root] = -1;

    while (currentNode != -1)
    {
        [branch]
        if (state == 0)
        {
            //NodeBuffer[currentNode].GlobalMinim = NodeBuffer[currentNode].Minim;
            //NodeBuffer[currentNode].GlobalMaxim = NodeBuffer[currentNode].Maxim;

            // Updating preorder linkage
            if (lastAnalized != -1)
                PreorderBuffer[lastAnalized] = currentNode;
            lastAnalized = currentNode;

            // Updating skipping linkage
            [branch]
            if (firstUp != -1)
            {
                while (firstUp != firstDown)
                {
                    SkipBuffer[firstUp] = currentNode;
                    firstUp = NodeBuffer[firstUp].Parent;
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
            if (firstUp == -1)
            {
                firstUp = currentNode;
                firstDown = -1;
            }
            GoParent(currentNode, state);
        }
        else
        {
            int childToGo = NodeBuffer[currentNode].Children[state != 1];// ? NodeBuffer[currentNode].LeftChild : NodeBuffer[currentNode].RightChild;

            [branch]
            if (childToGo != -1)
            {
                currentNode = childToGo;
                state = 0;
            }
            else
                state++;
        }
    }

    PreorderBuffer[lastAnalized] = -1;

    while (firstUp != -1)
    {
        SkipBuffer[firstUp] = -1;
        firstUp = NodeBuffer[firstUp].Parent;
    }
}

void main(float4 proj : SV_POSITION)
{
    uint2 crd = uint2(proj.xy);

    int root = RootBuffer[crd];
    if (root != -1)
        ResolveReferencesToNextNonChildrenInPreorder(root);
}