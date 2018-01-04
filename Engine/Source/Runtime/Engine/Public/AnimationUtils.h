// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimationUtils.h: Skeletal mesh animation utilities.
=============================================================================*/ 

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequence.h"

class UAnimCompress;
class UAnimSet;
struct FAnimCompressContext;

ENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogMatinee, Warning, All);
ENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogAnimNotify, Warning, All);
ENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogAnimMontage, Warning, All);
ENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogAnimMarkerSync, Warning, All);

// Forward declarations.
class UAnimCompress;
class UAnimSet;
struct FTransform;

/**
 * Encapsulates commonly useful data about bones.
 */
class FBoneData
{
public:
	FQuat		Orientation;
	FVector		Position;
	/** Bone name. */
	FName		Name;
	/** Direct descendants.  Empty for end effectors. */
	TArray<int32> Children;
	/** List of bone indices from parent up to root. */
	TArray<int32>	BonesToRoot;
	/** List of end effectors for which this bone is an ancestor.  End effectors have only one element in this list, themselves. */
	TArray<int32>	EndEffectors;
	/** If a Socket is attached to that bone */
	bool		bHasSocket;
	/** If matched as a Key end effector */
	bool		bKeyEndEffector;

	/**	@return		Index of parent bone; -1 for the root. */
	int32 GetParent() const
	{
		return GetDepth() ? BonesToRoot[0] : -1;
	}
	/**	@return		Distance to root; 0 for the root. */
	int32 GetDepth() const
	{
		return BonesToRoot.Num();
	}
	/** @return		true if this bone is an end effector (has no children). */
	bool IsEndEffector() const
	{
		return Children.Num() == 0;
	}
};

/**
 * Error statistics for an animation generated by TallyErrorsFromPerturbation, representing errors in translation or rotation due to a small incremental translation or rotation
 */
struct FAnimPerturbationError
{
	float MaxErrorInTransDueToTrans;
	float MaxErrorInRotDueToTrans;
	float MaxErrorInScaleDueToTrans;
	float MaxErrorInTransDueToRot;
	float MaxErrorInRotDueToRot;
	float MaxErrorInScaleDueToRot;
	float MaxErrorInTransDueToScale;
	float MaxErrorInRotDueToScale;
	float MaxErrorInScaleDueToScale;
};

/**
 * A set of error statistics for an animation, gathered by FAnimationUtils::ComputeCompressionError
 */
struct AnimationErrorStats
{
	/** Average world-space translation error across all end-effectors **/
	float AverageError;
	/** The worst error encountered across all end effectors **/
	float MaxError;
	/** Time at which the worst error occurred */
	float MaxErrorTime;
	/** Bone on which the worst error occurred */
	int32 MaxErrorBone;

	AnimationErrorStats()
		: AverageError(0.f)
		, MaxError(0.f)
		, MaxErrorTime(0.f)
		, MaxErrorBone(0)
	{}
};

/** Different ways to create an additive animation */
enum EConvertToAdditive
{
	/** Delta against SkelMesh Reference pose */
	CTA_RefPose,
	/** Delta against the first frame of another animation */
	CTA_AnimFirstFrame,
	/** Delta against another animation. Other animation scaled to match first anim's length */
	CTA_AnimScaled,
	CTA_MAX	// Always last one
};

/**
 * A collection of useful functions for skeletal mesh animation.
 */
class FAnimationUtils
{
public:


	/**
	* Builds the local-to-component matrix for the specified bone.
	*/
	static void BuildComponentSpaceTransform(FTransform& OutTransform,
												int32 BoneIndex,
												const TArray<FTransform>& BoneSpaceTransforms,
												const TArray<FBoneData>& BoneData);

	static void BuildSkeletonMetaData(USkeleton* Skeleton, TArray<FBoneData>& OutBoneData);


	/**
	 * Utility function to measure the accuracy of a compressed animation. Each end-effector is checked for 
	 * world-space movement as a result of compression.
	 *
	 * @param	AnimSet		The animset to calculate compression error for.
	 * @param	BoneData	BoneData array describing the hierarchy of the animated skeleton
	 * @param	ErrorStats	Output structure containing the final compression error values
	 * @return				None.
	 */
	static void ComputeCompressionError(const UAnimSequence* AnimSeq, const TArray<FBoneData>& BoneData, AnimationErrorStats& ErrorStats);

	/**
	 * Utility function to compress an animation. If the animation is currently associated with a codec, it will be used to 
	 * compress the animation. Otherwise, the default codec will be used. The CompressContext can specify whether an
	 * alternative compression codec can be used. If the alternative codec produces better compression and the accuracy
	 * of the compressed animation remains within tolerances, the alternative codec will be used. 
	 * See GetAlternativeCompressionThreshold for information on the tolerance value used.
	 *
	 * @param	AnimSeq				The anim sequence to compress.
	 * @param	CompressContext		Context for applying compression, tracking memory saved etc
	 * @return						None.
	 */
	ENGINE_API static void CompressAnimSequence(UAnimSequence* AnimSeq, FAnimCompressContext& CompressContext);

	/**
	 * Utility function to compress an animation. If the animation is currently associated with a codec, it will be used to
	 * compress the animation. Otherwise, the default codec will be used. The CompressContext can specify whether an
	 * alternative compression codec can be used. If the alternative codec produces better compression and the accuracy
	 * of the compressed animation remains within tolerances, the alternative codec will be used.
	 * See GetAlternativeCompressionThreshold for information on the tolerance value used.
	 *
	 * @param	AnimSeq									The anim sequence to compress.
	 * @param	CompressContext							Context for applying compression, tracking memory saved etc*
	 * @param	MasterTolerance							The alternate error threshold (0.0 means don't try anything other than the current / default scheme)
	 * @param	bFirstRecompressUsingCurrentOrDefault	If true, then the animation will be first recompressed with it's current compressor if non-NULL, or with the global default compressor (specified in the engine ini)
	 * @param	bForceBelowThreshold					If true and the existing compression error is greater than MasterTolerance, then any compression technique (even one that increases the size) with a lower error will be used until it falls below the threshold
	 * @param	bRaiseMaxErrorToExisting				If true and the existing compression error is greater than MasterTolerance, then MasterTolerance will be effectively raised to the existing error level
	 * @param	bTryFixedBitwiseCompression				If true, the uniform bitwise techniques will be tried
	 * @param	bTryPerTrackBitwiseCompression			If true, the per-track compressor techniques will be tried
	 * @param	bTryLinearKeyRemovalCompression			If true, the linear key removal techniques will be tried
	 * @param	bTryIntervalKeyRemoval					If true, the resampling techniques will be tried
	 *
	 * @return	None.
	 */
	static void CompressAnimSequenceExplicit(
		UAnimSequence* AnimSeq,
		FAnimCompressContext& CompressContext,
		float MasterTolerance,
		bool bFirstRecompressUsingCurrentOrDefault,
		bool bForceBelowThreshold,
		bool bRaiseMaxErrorToExisting,
		bool bTryFixedBitwiseCompression,
		bool bTryPerTrackBitwiseCompression,
		bool bTryLinearKeyRemovalCompression,
		bool bTryIntervalKeyRemoval);

	/**
	 * Tests for a missing or invalid mesh on the animation sequence, warning if one or more were found
	 *
	 * @param AnimSeq	The anim sequence to check.
	 *
	 * @return	None.
	 */
	static void TestForMissingMeshes(UAnimSequence* AnimSeq);

	/**
	 * Determines the current setting for world-space error tolerance in the animation compressor.
	 * When requested, animation being compressed will also consider an alternative compression
	 * method if the end result of that method produces less error than the AlternativeCompressionThreshold.
	 * The default tolerance value is 0.0f (no alternatives allowed) but may be overridden using a field in the base engine INI file.
	 *
	 * @return				World-space error tolerance for considering an alternative compression method
	 */
	static float GetAlternativeCompressionThreshold();
	
	/**
	 * Determines the current setting for recompressing all animations upon load. The default value 
	 * is False, but may be overridden by an optional field in the base engine INI file. 
	 *
	 * @return				true if the engine settings request that all animations be recompiled
	 */
	static bool GetForcedRecompressionSetting();

	/** Get default Outer for AnimSequences contained in this AnimSet.
	 *  The intent is to use that when Constructing new AnimSequences to put into that set.
	 *  The Outer will be Package.<AnimSetName>_Group.
	 *  @param bCreateIfNotFound if true, Group will be created. This is only in the editor.
	 */
	static UObject* GetDefaultAnimSequenceOuter(UAnimSet* InAnimSet, bool bCreateIfNotFound);

	/**
	 * Converts an animation codec format into a human readable string
	 *
	 * @param	InFormat	The format to convert into a string
	 * @return				The format as a string
	 */
	static FString GetAnimationKeyFormatString(enum AnimationKeyFormat InFormat);

	/**
	 * Converts an animation compression type into a human readable string
	 *
	 * @param	InFormat	The compression format to convert into a string
	 * @return				The format as a string
	 */
	ENGINE_API static FString GetAnimationCompressionFormatString(enum AnimationCompressionFormat InFormat);

	/**
	 * Computes the 'height' of each track, relative to a given animation linkup.
	 *
	 * The track height is defined as the minimal number of bones away from an end effector (end effectors are 0, their parents are 1, etc...)
	 *
	 * @param AnimLinkup			The animation linkup
	 * @param BoneData				The bone data to check
	 * @param NumTracks				The number of tracks
	 * @param TrackHeights [OUT]	The computed track heights
	 *
	 */
	static void CalculateTrackHeights(UAnimSequence* AnimSeq, const TArray<FBoneData>& BoneData, int NumTracks, TArray<int32>& TrackHeights);

	/**
	 * Checks a set of key times to see if the spacing is uniform or non-uniform.
	 * Note: If there are as many times as frames, they are automatically assumed to be uniformly spaced.
	 * Note: If there are two or fewer times, they are automatically assumed to be uniformly spaced.
	 *
	 * @param AnimSeq		The animation sequence the Times array is associated with
	 * @param Times			The array of key times
	 *
	 * @return				true if the keys are uniformly spaced (or one of the trivial conditions is detected).  false if any key spacing is greater than 1e-4 off.
	 */
	static bool HasUniformKeySpacing(UAnimSequence* AnimSeq, const TArray<float>& Times);

	/**
	 * Perturbs the bone(s) associated with each track in turn, measuring the maximum error introduced in end effectors as a result
	 */
	static void TallyErrorsFromPerturbation(
		const UAnimSequence* AnimSeq,
		int32 NumTracks,
		const TArray<FBoneData>& BoneData,
		const FVector& PositionNudge,
		const FQuat& RotationNudge,
		const FVector& ScaleNudge,
		TArray<FAnimPerturbationError>& InducedErrors);

	/**
	 * @return		The default animation compression algorithm singleton, instantiating it if necessary.
	 */
	ENGINE_API static UAnimCompress* GetDefaultAnimationCompressionAlgorithm();

};
