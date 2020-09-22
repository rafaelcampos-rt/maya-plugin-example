
#include "DamperNode.h"
#include <maya/MFnPlugin.h>
#include <maya/MNodeCacheDisablingInfoHelper.h>

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>

#include <cassert>
#include <limits>

MObject DamperNode::mTarget;
MObject DamperNode::mDampingFactor;
MObject DamperNode::mCurrentTime;
MObject DamperNode::mSimulationEnabled;
MObject DamperNode::mSimulationStartTime;
MObject DamperNode::mOutput;
MObject DamperNode::mPreviousOutput;
MTypeId DamperNode::mId{ 0x00136100};


MStatus DamperNode::compute(const MPlug &plug, MDataBlock &data) {
    MStatus returnStatus;

    if (plug == mOutput)
    {
        assert(!mTarget.isNull());
        const float targetValue = data.inputValue(mTarget, &returnStatus).asFloat();
        assert(returnStatus);

        assert(!mSimulationEnabled.isNull());
        bool simulationEnabledValue = data.inputValue(mSimulationEnabled, &returnStatus).asBool();
        assert(returnStatus);

        if (simulationEnabledValue)
        {
            assert(!mCurrentTime.isNull());
            const MTime currentTime = data.inputValue(mCurrentTime, &returnStatus).asTime();
            assert(returnStatus);

            assert(!mSimulationStartTime.isNull());
            const MTime simulationStartTime = data.inputValue(mSimulationStartTime, &returnStatus).asTime();
            assert(returnStatus);

            simulationEnabledValue = (simulationStartTime < currentTime);
        }

        float outputValue = targetValue;
        if (simulationEnabledValue)
        {
            assert(!mDampingFactor.isNull());
            const float dampingFactorValue = data.inputValue(mDampingFactor, &returnStatus).asFloat();
            assert(returnStatus);

            assert(!mPreviousOutput.isNull());
            const float previousOutputValue = data.inputValue(mPreviousOutput, &returnStatus).asFloat();
            assert(returnStatus);

            outputValue = previousOutputValue + (targetValue - previousOutputValue) * dampingFactorValue;
        }

        MDataHandle outputHandle = data.outputValue(mOutput);
        outputHandle.set(outputValue);
        data.setClean(plug);

        MDataHandle previousOutputHandle = data.outputValue(mPreviousOutput);
        previousOutputHandle.set(outputValue);
        data.setClean(mPreviousOutput);
    }
    else
    {
        return MS::kUnknownParameter;
    }

    return MS::kSuccess;
}
void DamperNode::getCacheSetup(const MEvaluationNode& node,
                                     MNodeCacheDisablingInfo &info,
                                     MNodeCacheSetupInfo &setupInfo,
                                     MObjectArray &monitoredAttributes) const {
    const bool requestSimulation = MNodeCacheDisablingInfoHelper::testBooleanAttribute(
        nullptr, monitoredAttributes, node, mSimulationEnabled, false);
    if (requestSimulation)
    {
        setupInfo.setRequirement(MNodeCacheSetupInfo::kSimulationSupport, true);
    }
}

MTimeRange DamperNode::transformInvalidationRange(const MPlug &source, const MTimeRange &input) const
{
    static constexpr MTime::MTick kMaxTimeTick = std::numeric_limits<MTime::MTick>::max()/2;
    static constexpr MTime::MTick kMinTimeTick = std::numeric_limits<MTime::MTick>::max()/2 + 1;
    static const MTime kMaxTime {kMaxTimeTick / static_cast<double>(MTime::ticksPerSecond()), MTime::kSeconds};
    static const MTime kMinTime { kMinTimeTick / static_cast<double>(MTime::ticksPerSecond()), MTime::kSeconds};

    MDataBlock data = const_cast<DamperNode*>(this)->forceCache();
    if (!data.isClean(mSimulationStartTime) || !data.isClean(mSimulationEnabled))
    {
        return MTimeRange{kMinTime, kMaxTime};
    }

    MStatus returnStatus;
    MDataHandle simulationEnableData = data.inputValue(mSimulationEnabled, &returnStatus);
    const bool simulationEnabled = returnStatus ? simulationEnableData.asBool() : true;
    if (!simulationEnabled)
        return input;

    MDataHandle simulationStartTimeData = data.inputValue(mSimulationStartTime, &returnStatus);
    const MTime simulationStartTime = returnStatus ? simulationStartTimeData.asTime() : kMinTime;
    const MTime simulationEndTime = kMaxTime;

    if (input.intersects(simulationStartTime, simulationEndTime))
    {
        return input | MTimeRange{simulationStartTime, simulationEndTime};
    }
    else
    {
        return MTimeRange{};
    }
}
void *DamperNode::creator() {
    return new DamperNode();
}
MStatus DamperNode::initialize() {
    MFnNumericAttribute numericAttribute;
    MFnUnitAttribute unitAttribute;
    MStatus status;

    mTarget = numericAttribute.create("target", "tgt", MFnNumericData::kFloat, 0.0);
    numericAttribute.setStorable(true);

    mDampingFactor = numericAttribute.create("dampingFactor", "df", MFnNumericData::kFloat, 0.1);
    numericAttribute.setStorable(true);

    mCurrentTime = unitAttribute.create("currentTime", "ct", MFnUnitAttribute::kTime, 0.0);
    unitAttribute.setWritable(true);
    unitAttribute.setStorable(true);
    unitAttribute.setReadable(true);
    unitAttribute.setKeyable(true);

    mSimulationEnabled = numericAttribute.create("simulationEnabled", "se", MFnNumericData::kBoolean, 1.0);
    numericAttribute.setStorable(true);

    mSimulationStartTime = unitAttribute.create("simulationStartTime", "sst", MFnUnitAttribute::kTime, 0.0);
    unitAttribute.setStorable(true);

    mOutput = numericAttribute.create("output", "out", MFnNumericData::kFloat, 0.0);
    numericAttribute.setWritable(false);
    numericAttribute.setStorable(false);

    mPreviousOutput = numericAttribute.create("previousOutput", "po", MFnNumericData::kFloat, 0.0);
    numericAttribute.setWritable(false);
    numericAttribute.setStorable(false);
    numericAttribute.setHidden(true);

    status = addAttribute(mTarget);
    assert(status);

    status = addAttribute(mDampingFactor);
    assert(status);

    status = addAttribute(mCurrentTime);
    assert(status);

    status = addAttribute(mSimulationEnabled);
    assert(status);

    status = addAttribute(mSimulationStartTime);
    assert(status);

    status = addAttribute(mOutput);
    assert(status);

    status = addAttribute(mPreviousOutput);
    assert(status);

    status = attributeAffects(mTarget, mOutput);
    assert(status);
    status = attributeAffects(mDampingFactor, mOutput);
    assert(status);
    status = attributeAffects(mCurrentTime, mOutput);
    assert(status);
    status = attributeAffects(mSimulationEnabled, mOutput);
    assert(status);
    status = attributeAffects(mSimulationStartTime, mOutput);
    assert(status);

    return MS::kSuccess;
}

// Plug-in entry points, as required by Maya
MStatus initializePlugin(MObject object) {
    MFnPlugin damperPlugin(object, "Rafael Campos", "1.0", "Any");
    MStatus status;

    status = damperPlugin.registerNode(
        "DamperNode",
        DamperNode::mId,
        DamperNode::creator,
        DamperNode::initialize
    );

    if (!status)
        status.perror("Registering node - DamperNode");

    return status;
}

MStatus uninitializePlugin(MObject object)
{
    MStatus status;
    MFnPlugin plugin(object);

    status = plugin.deregisterNode(DamperNode::mId);

    if (!status)
    {
        status.perror("De-registering node - DamperNode");
        return status;
    }

    return status;
}