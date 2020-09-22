//
// Created by rafael on 8/21/20.
//

#ifndef DAMPERNODE__DAMPERNODE_H
#define DAMPERNODE__DAMPERNODE_H

#include <maya/MPxNode.h>

class DamperNode : public MPxNode {
public:
  MStatus compute(const MPlug &Plug, MDataBlock &Block) override;

  void getCacheSetup(const MEvaluationNode &Node,
                     MNodeCacheDisablingInfo &Info,
                     MNodeCacheSetupInfo &SetupInfo,
                     MObjectArray &Array) const override;

  MTimeRange transformInvalidationRange(const MPlug &source, const MTimeRange &input) const override;

  static void *creator();

  static MStatus initialize();

  static MObject mTarget;
  static MObject mDampingFactor;
  static MObject mCurrentTime;
  static MObject mSimulationEnabled;
  static MObject mSimulationStartTime;
  static MObject mOutput;
  static MObject mPreviousOutput;

  static MTypeId mId; //(0x00136100);
};

#endif //DAMPERNODE__DAMPERNODE_H
