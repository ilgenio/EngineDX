#include "Globals.h"

#include "Model.h"
#include "Mesh.h"
#include "Material.h"
#include "Scene.h"
#include "AnimationClip.h"
#include "QuadTree.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#pragma warning(push)
#pragma warning(disable : 4018) 
#pragma warning(disable : 4267) 
#include "tiny_gltf.h"
#pragma warning(pop)

#include "gltf_utils.h"


Model::Model(Scene* parentScene, const char* name) : scene(parentScene), name(name)
{
}

Model::~Model()
{
    for (Mesh* mesh : meshes) delete mesh;
    for (Material* material : materials) delete material;
    for (Node* node : nodes) delete node;
    for (MeshInstance* instance : instances) delete instance;
    for (Skin* skin : skins) delete skin;

    scene->onRemoveModel(this);   
}

bool Model::load(const tinygltf::Model &srcModel, const char *basePath)
{
    std::vector<int> materialMappings;
    materialMappings.reserve(srcModel.meshes.size());

    std::vector<std::pair<UINT, UINT>> meshMappings;
    meshMappings.reserve(srcModel.meshes.size());

    for (const auto& srcMesh : srcModel.meshes)
    {
        meshMappings.push_back({ int(materialMappings.size()), int(srcMesh.primitives.size()) });

        for (const auto& primitive : srcMesh.primitives)
        {
            Mesh* mesh = new Mesh;
            mesh->load(srcModel, srcMesh, primitive);

            meshes.push_back(mesh);

            materialMappings.push_back(primitive.material);            
        }
    }

    _ASSERTE(meshes.size() == materialMappings.size());


    for(const auto& srcMaterial : srcModel.materials)
    {
        Material* material = new Material(this, srcMaterial.name.c_str());

        material->load(srcModel, srcMaterial, basePath);

        materials.emplace_back(material);
    }


    for(const tinygltf::Skin& srcSkin : srcModel.skins)
    {   
        Skin* skin = new Skin;

        skin->numJoints = UINT(srcSkin.joints.size());
        skin->jointNodeIndices = std::make_unique<UINT[]>(skin->numJoints);
        skin->inverseBindMatrices = std::make_unique<Matrix[]>(skin->numJoints);

        for (UINT i = 0; i < skin->numJoints; ++i)
        {
            skin->jointNodeIndices[i] = srcSkin.joints[i];
        }

        if (srcSkin.inverseBindMatrices >= 0)
        {
            UINT numMatrices = 0;
            bool ok = loadAccessorTyped(skin->inverseBindMatrices, numMatrices, srcModel, srcSkin.inverseBindMatrices);
            _ASSERTE(ok && numMatrices == skin->numJoints);
        }

        skins.push_back(skin);
    }

    std::vector<UINT> nodeMapping;
    nodeMapping.resize(srcModel.nodes.size(), UINT(-1));

    for(const tinygltf::Scene& scene : srcModel.scenes)
    {
        for (int nodeIndex : scene.nodes)
        {
            generateNodes(srcModel, nodeIndex, -1, meshMappings, materialMappings, nodeMapping);
        }
    }

    // Remap skin joint node indices
    for (Skin* skin : skins)
    {
        for (UINT i = 0; i < skin->numJoints; ++i)
        {
            UINT originalNodeIndex = skin->jointNodeIndices[i];
            _ASSERTE(nodeMapping[originalNodeIndex] != UINT(-1));

            skin->jointNodeIndices[i] = nodeMapping[originalNodeIndex];
        }
    }

    return true;

}

UINT Model::generateNodes(const tinygltf::Model &model, UINT nodeIndex, INT parentIndex,
                   const std::vector<std::pair<UINT, UINT>> &meshMapping,
                   const std::vector<int> &materialMapping,
                   std::vector<UINT>& nodeMapping)
{
    const tinygltf::Node& node = model.nodes[nodeIndex];

    Matrix local = Matrix::Identity;

    if (node.matrix.size() == 16)
    {
        float* ptr = reinterpret_cast<float*>(&local);
        for (UINT i = 0; i < 16; ++i) ptr[i] = float(node.matrix[i]);

        // Column major + post multiply  So don't need to be transposed because we are row major + pre multiply
    }
    else
    {
        Vector3 translation = Vector3::Zero;
        Vector3 scale = Vector3::One;
        Quaternion rotation = Quaternion::Identity;

        if (node.translation.size() == 3) translation = Vector3(float(node.translation[0]), float(node.translation[1]), float(node.translation[2]));
        if (node.rotation.size() == 4) rotation = Quaternion(float(node.rotation[0]), float(node.rotation[1]), float(node.rotation[2]), float(node.rotation[3]));
        if (node.scale.size() == 3) scale = Vector3(float(node.scale[0]), float(node.scale[1]), float(node.scale[2]));

        local = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(translation);
    }

    // Note: nodes are guaranteed to be stored in preorder

    Node* dst = new Node;
    dst->localTransform = local;
    dst->dirtyWorld = true;
    dst->name = node.name;
    dst->parent = parentIndex;
    dst->numChilds = UINT(node.children.size());

    UINT realIndex = UINT(nodes.size());

    nodeMapping[nodeIndex] = realIndex;

    if (node.mesh >= 0)
    {
        // as we have one mesh per primitive one gltf mesh generates more than one of our meshes
        for(UINT i = 0; i < meshMapping[node.mesh].second; ++i)
        {
            // TODO: assign pointers instead of indices
            MeshInstance* instance = new MeshInstance;
            instance->meshIndex     = UINT(meshMapping[node.mesh].first + i);
            instance->materialIndex = UINT(materialMapping[meshMapping[node.mesh].first + i]);
            instance->nodeIndex     = realIndex;
            instance->skinIndex     = node.skin;

            if(instance->skinIndex >= 0)
            {
                _ASSERTE(instance->skinIndex < skins.size());

                instance->palette = std::make_unique<Matrix[]>(skins[instance->skinIndex]->numJoints);
            }
            
            _ASSERTE(instance->meshIndex < meshes.size());
            _ASSERTE(instance->materialIndex < materials.size());

            instances.push_back(instance);
        }
    }

    nodes.push_back(dst);

    for (int childIndex : node.children)
    {
        dst->numChilds += generateNodes(model, childIndex, realIndex, meshMapping, materialMapping, nodeMapping);
    }

    return dst->numChilds;

}

void Model::updateWorldTransforms()
{
    UINT numDirty = 0;

    for(Node* node : nodes)
    {
        bool dirty = numDirty > 0;

        if(dirty)
        {
            _ASSERT(numDirty > node->numChilds);
            --numDirty;
        } 
        else if(node->dirtyWorld)
        {
            // if a node is dirty, all its childs are dirty too
            dirty = true;
            numDirty = node->numChilds;
        }

        if (dirty)
        {
            INT parentIndex = node->parent;
            if (parentIndex >= 0)
            {
                Node* parent = nodes[parentIndex];
                _ASSERTE(parent->dirtyWorld == false);

                // Parent is clean, so we can use its world transform directly
                node->worldTransform = node->localTransform * parent->worldTransform;
            }
            else
            {
                // No parent, so local is world
                node->worldTransform = node->localTransform;
            }

            node->dirtyWorld = false;
        }

        node->lastFrameDirty = dirty;
    }

}

void Model::updateQuadTree(QuadTree* quadTree, bool force) 
{
    for (MeshInstance* instance : instances)
    {
        Node* node = nodes[instance->nodeIndex];

        if (force || node->lastFrameDirty)
        {
            _ASSERTE(instance->meshIndex < meshes.size());

            const Mesh* mesh = meshes[instance->meshIndex];
            BoundingOrientedBox worldBox;
            BoundingOrientedBox::CreateFromBoundingBox(worldBox, mesh->getBoundingBox());
            worldBox.Transform(worldBox, node->worldTransform);
            
            UINT cellIndex = quadTree->computeCellIndex(worldBox);
            _ASSERTE(cellIndex <= quadTree->getCellCount());

            if (cellIndex != instance->quadTreeCell)
            {
                if (instance->quadTreeCell < quadTree->getCellCount()) quadTree->removeObject(instance->quadTreeCell);
                if (cellIndex < quadTree->getCellCount()) quadTree->addObject(cellIndex);

                instance->quadTreeCell = cellIndex;
            }
        }
    }
}

void Model::frustumCulling(const Vector4 frustumPlanes[6], const Vector3 absFrustumPlanes[6], 
                           const std::vector<IntersectionType>& containment, 
                           std::vector<RenderMesh>& renderList) const
{
    for (const MeshInstance* instance : instances)
    {
        _ASSERTE(instance->meshIndex < meshes.size());
        _ASSERTE(instance->materialIndex < materials.size());
        _ASSERTE(instance->nodeIndex < nodes.size());

        bool addInstance = false;

        if (instance->quadTreeCell < containment.size())
        {
            _ASSERTE(instance->quadTreeCell < containment.size());

            if (containment[instance->quadTreeCell] == INSIDE)
            {
                addInstance = true;
            }
            else if(containment[instance->quadTreeCell] == INTERSECTION)
            {
                BoundingOrientedBox worldBox;
                BoundingOrientedBox::CreateFromBoundingBox(worldBox, meshes[instance->meshIndex]->getBoundingBox());
                worldBox.Transform(worldBox, nodes[instance->nodeIndex]->worldTransform);

                addInstance = insidePlanes(frustumPlanes, absFrustumPlanes, worldBox) != OUTSIDE;
            }
        }

        if (addInstance)
        {
            RenderMesh renderMesh;
            renderMesh.mesh = meshes[instance->meshIndex];
            renderMesh.material = materials[instance->materialIndex];

            _ASSERTE(nodes[instance->nodeIndex]->dirtyWorld == false);

            renderMesh.transform = nodes[instance->nodeIndex]->worldTransform;

            renderMesh.normalMatrix = renderMesh.transform;
            renderMesh.normalMatrix.Translation(Vector3::Zero);
            renderMesh.normalMatrix.Invert();
            renderMesh.normalMatrix.Transpose();

            if(instance->skinIndex >= 0 && renderMesh.mesh->needsSkinning())
            {
                updateSkinningMatrices(instance);

                renderMesh.palette   = instance->palette.get();
                renderMesh.numJoints = skins[instance->skinIndex]->numJoints;
            }

            renderList.push_back(renderMesh);
        }
    }
}

void Model::PlayAnim(std::shared_ptr<AnimationClip> clip, float fadeIn /*= 0.0f*/)
{
    if (!clip) return;

    std::unique_ptr<AnimInstance> newAnim = std::make_unique<AnimInstance>();
    newAnim->clip = clip;
    newAnim->fadeIn = fadeIn;
    newAnim->time = 0.0f;

    newAnim->next = std::move(currentAnim);
    currentAnim = std::move(newAnim);
}

void Model::StopAnim()
{
    currentAnim.reset();
}

void Model::updateAnim(float deltaTime)
{
    std::vector<AnimInstance*> anims;

    AnimInstance* anim = currentAnim.get();

    while (anim)
    {
        anims.push_back(anim);

        anim->time += deltaTime;

        // Loop animation
        while (anim->time > anim->clip->getDuration())
        {
            anim->time -= anim->clip->getDuration();
        }

        if (anim->time > anim->fadeIn)
        {
            anim->next.release();
            anim = nullptr;
        }
        else
        {
            anim = anim->next.get();
        }
    }

    // update nodes

    for(Node* node : nodes)
    {
        Vector3 nodePos = Vector3::Zero;
        Quaternion nodeRot = Quaternion::Identity;
        bool updated = false;

        for(auto it = anims.rbegin(); it != anims.rend(); ++it)
        {
            AnimInstance* anim = *it;

            Vector3 pos = Vector3::Zero;
            Quaternion rot = Quaternion::Identity;

            if(anim->clip->getPosRot(node->name, anim->time, pos, rot))
            {
                updated = true;

                if(anim->next)
                {
                    float t = std::min(anim->time / anim->fadeIn, 1.0f);

                    nodePos = Vector3::Lerp(nodePos, pos, t);
                    nodeRot = Quaternion::Lerp(nodeRot, rot, t);
                }
                else 
                {
                    nodePos = pos;
                    nodeRot = rot;
                }
            }

        }

        if (updated)
        {
            node->localTransform = Matrix::CreateFromQuaternion(nodeRot) * Matrix::CreateTranslation(nodePos);
            node->dirtyWorld = true;
        }
    }

    for (MeshInstance* instance : instances)
    {
        instance->dirtyPalette = instance->skinIndex >= 0;

        if(instance->skinIndex >= 0)
        {
            _ASSERTE(instance->skinIndex < skins.size());

            const Skin* skin = skins[instance->skinIndex];

            for (UINT j = 0; j < skin->numJoints; ++j)
            {
                INT jointNodeIndex = skin->jointNodeIndices[j];
                _ASSERTE(jointNodeIndex >= 0 && UINT(jointNodeIndex) < nodes.size());

                const Node* jointNode = nodes[jointNodeIndex];

                instance->palette[j] =  jointNode->worldTransform * skin->inverseBindMatrices[j];
            }
        }
    }
}

void Model::updateSkinningMatrices(const MeshInstance* instance) const
{
    if(instance->skinIndex < 0 || !instance->dirtyPalette) return;

    _ASSERTE(instance->skinIndex < skins.size());

    const Skin* skin = skins[instance->skinIndex];

    for (UINT j = 0; j < skin->numJoints; ++j)
    {
        INT jointNodeIndex = skin->jointNodeIndices[j];
        _ASSERTE(jointNodeIndex >= 0 && UINT(jointNodeIndex) < nodes.size());

        const Node* jointNode = nodes[jointNodeIndex];

        instance->palette[j] = jointNode->worldTransform * skin->inverseBindMatrices[j];
    }

    instance->dirtyPalette = false;
}