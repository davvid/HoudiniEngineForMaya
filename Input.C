#include "Input.h"

#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnTypedAttribute.h>

#include <maya/MArrayDataBuilder.h>
#include <maya/MDataBlock.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MString.h>

#include <HAPI/HAPI.h>

#include "AssetNode.h"
#include "InputMesh.h"
#include "InputCurve.h"
#include "InputParticle.h"
#include "hapiutil.h"
#include "util.h"

Inputs::Inputs(HAPI_NodeId nodeId) :
    myNodeId(nodeId)
{
}

Inputs::~Inputs()
{
    for(AssetInputVector::iterator iter = myAssetInputs.begin();
            iter != myAssetInputs.end();
            iter++)
    {
        delete *iter;
        *iter = NULL;
    }
    myAssetInputs.clear();
}

MStatus
Inputs::compute(MDataBlock &dataBlock)
{
    MStatus status;

    MArrayDataHandle inputArrayHandle = dataBlock.outputArrayValue(AssetNode::input, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MArrayDataBuilder inputArrayBuilder = inputArrayHandle.builder(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for(unsigned int i = 0; i < myAssetInputs.size(); ++i)
    {
        MDataHandle inputHandle = inputArrayBuilder.addElement(i, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);


        MDataHandle inputNameHandle = inputHandle.child(AssetNode::inputName);

        HAPI_StringHandle nameSH;
        HAPI_GetNodeInputName(
                Util::theHAPISession.get(),
                myNodeId,
                i,
                &nameSH);

        inputNameHandle.set(Util::HAPIString(nameSH));
    }

    status = inputArrayHandle.set(inputArrayBuilder);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    return MStatus::kSuccess;
}

void Inputs::setNumInputs(int numInputs)
{
    // delete any Input that will be removed
    for(unsigned int i = numInputs; i < myAssetInputs.size(); i++)
    {
        Input* &assetInput = myAssetInputs[i];

        if(assetInput)
        {
            delete assetInput;
            assetInput = NULL;
        }
    }

    myAssetInputs.resize(numInputs, NULL);
}

Input::Input() :
    myGeometryNodeId(-1)
{
}

Input::~Input()
{
}

Input*
Input::createAssetInput(AssetInputType assetInputType)
{
    Input* assetInput = NULL;
    switch(assetInputType)
    {
        case AssetInputType_Mesh:
            assetInput = new InputMesh();
            break;
        case AssetInputType_Curve:
            assetInput = new InputCurve();
            break;
        case AssetInputType_Particle:
            assetInput = new InputParticle();
            break;
        case AssetInputType_Invalid:
            break;
    }
    return assetInput;
}

void
Input::setInputName(
        HAPI_NodeId inputNodeId,
        HAPI_PartId inputPartId,
        HAPI_AttributeOwner owner, int count,
        const MPlug &plug
        )
{
    MPlug sourcePlug = Util::plugSource(plug);
    MObject sourceNodeObj = sourcePlug.node();

    MString name = Util::getNodeName(sourceNodeObj);

    if(owner == HAPI_ATTROWNER_PRIM)
    {
        CHECK_HAPI(hapiSetPrimAttribute(
                    inputNodeId, inputPartId,
                    1,
                    "name",
                    std::vector<const char*>(count, name.asChar())
                    ));
    }
    else if(owner == HAPI_ATTROWNER_POINT)
    {
        CHECK_HAPI(hapiSetPointAttribute(
                    inputNodeId, inputPartId,
                    1,
                    "name",
                    std::vector<const char*>(count, name.asChar())
                    ));
    }
}

void
Input::setInputTransform(MDataHandle &dataHandle)
{
    MMatrix transformMatrix = dataHandle.asMatrix();

    float matrix[16];
    transformMatrix.get(reinterpret_cast<float(*)[4]>(matrix));

    HAPI_TransformEuler transformEuler;
    HAPI_ConvertMatrixToEuler(
            Util::theHAPISession.get(),
            matrix,
            HAPI_SRT,
            HAPI_XYZ,
            &transformEuler
            );

    CHECK_HAPI(HAPI_SetObjectTransform(
            Util::theHAPISession.get(),
            transformNodeId(),
            &transformEuler
            ));
}
