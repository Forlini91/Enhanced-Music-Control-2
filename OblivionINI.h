#pragma once

/* Oblivion INI for Oblivion version 1.2.0.416 only.

This header file contains all the INI settings that the game engine uses. However altering
a value may not effect the game immediately and may need you to restart the game. This is 
because some settings are only used when the game is initialized, some are used for 
constructing an object (once the object is constructed then the engine reads the value 
from within the object's namespace) and some are copied into another variable for use by 
the game's engine (if you want to alter the variable but not update the INI file with 
the new value).

Altering a value will cause the INI file to get updated with the new value when the game
exits.

I've declared all the variables as volatile as strictly speaking they could get altered
outside the scope of OBSE. I've also declared the pointers as constants so hopefully you
can't accidently write a value to the pointer instead of what it's pointing to.
*/

namespace INI {
	namespace General {
		volatile char **sStartingCell = 						(volatile char **)0x00B02CE0;
		volatile char **sStartingCellY = 						(volatile char **)0x00B02D00;
		volatile char **sStartingCellX = 						(volatile char **)0x00B02CF8;
		volatile char **sStartingWorld = 						(volatile char **)0x00B02CF0;
		volatile char **sTestFile10 = 							(volatile char **)0x00B02CD8;
		volatile char **sTestFile9 =							(volatile char **)0x00B02CD0;
		volatile char **sTestFile8 =							(volatile char **)0x00B02CC8;
		volatile char **sTestFile7 =							(volatile char **)0x00B02CC0;
		volatile char **sTestFile6 =							(volatile char **)0x00B02CB8;
		volatile char **sTestFile5 =							(volatile char **)0x00B02CB0;
		volatile char **sTestFile4 =							(volatile char **)0x00B02CA8;
		volatile char **sTestFile3 =							(volatile char **)0x00B02CA0;
		volatile char **sTestFile2 =							(volatile char **)0x00B02C98;
		volatile char **sTestFile1 =							(volatile char **)0x00B02C90;
		volatile bool *bEnableProfile =							(bool *)0x00B23C60;
		volatile bool *bDrawSpellContact =						(bool *)0x00B15A68;
		volatile bool *const bUseEyeEnvMapping =				(bool *)0x00B120E4;
		volatile bool *const bFixFaceNormals =					(bool *)0x00B120BC;
		volatile bool *const bUseFaceGenHeads =					(bool *)0x00B120B4;
		volatile bool *const bFaceMipMaps =						(bool *)0x00B11F8C;
		volatile bool *const bFaceGenTexturing =				(bool *)0x00B10D3C;
		volatile bool *const bDefaultCOCPlacement =				(bool *)0x00B097E0;
		volatile UInt32 *const uGridDistantTreeRange =			(UInt32 *)0x00B06A98;
		volatile UInt32 *const uGridDistantCount =				(UInt32 *)0x00B06A90;
		volatile UInt32 *const uGridsToLoad =					(UInt32 *)0x00B06A2C;
		volatile float *const fGlobalTimeMultiplier =			(float *)0x00B06704;
		volatile float *const fAnimationDefaultBlend =			(float *)0x00B06538;
		volatile float *const fAnimationMult =					(float *)0x00B06530;
		volatile bool *const bFixAIPackagesOnLoad =				(bool *)0x00B05DC0;
		volatile char **const sLocalSavePath =					(volatile char **)0x00B05564;
		volatile char **const sLocalMasterPath =				(volatile char **)0x00B0555C;
		volatile bool *const bTintMipMaps =						(bool *)0x00B05244;
		volatile UInt32 *const uInteriorCellBuffer =			(UInt32 *)0x00B051D4;
		volatile UInt32 *const uExteriorCellBuffer =			(UInt32 *)0x00B051DC;
		volatile int *const iIntroSequencePriority =			(int *)0x00B030C4;
		volatile bool *const bPreloadIntroSequence =			(bool *)0x00B030B4;
		volatile float *const fStaticScreenWaitTime =			(float *)0x00B030AC;
		volatile char **const sMainMenuMovie =					(volatile char **)0x00B03094;
		volatile char **const sMainMenuMovieIntro =				(volatile char **)0x00B0308C;
		volatile char **const sIntroSequence =					(volatile char **)0x00B03084;
		volatile int *const iFPSClamp =							(int *)0x00B02D10;
		volatile bool *const bActivateAllQuestScripts =			(bool *)0x00B10CA0;
		volatile char **const sMainMenuMusic =					(volatile char **)0x00B14374;
		volatile bool *const bExternalLODDataFiles =			(bool *)0x00B09DB0;
		volatile bool *const bBorderRegionsEnabled =			(bool *)0x00B14F58;
		volatile bool *const bDisableHeadTracking = 			(bool *)0x00B148F4;
		volatile bool *const bTrackAllDeaths =					(bool *)0x00B148C4;
		volatile char **const sCharGenQuest =					(volatile char **)0x00B1436C;
		volatile UInt32 *const uiFaceGenMaxEGTDataSize =		(UInt32 *)0x00B120EC;
		volatile UInt32 *const uiFaceGenMaxEGMDataSize =		(UInt32 *)0x00B120F4;
		volatile char **const sBetaCommentFileName =			(volatile char **)0x00B09EF0;
		volatile bool *const bCheckCellOffsetsOnInit =			(bool *)0x00B09DB8;
		volatile bool *const bCreateShaderPackage =				(bool *)0x00B06F74;
		volatile UInt32 *const uGridDistantTreeRangeCity =		(UInt32 *)0x00B06AA8;
		volatile UInt32 *const uGridDistantCountCity =			(UInt32 *)0x00B06AA0;
		volatile bool *const bWarnOnMissingFileEntry =			(bool *)0x00B06310;
		volatile int *const iSaveGameBackupCount =				(int *)0x00B05BC4;
		volatile bool *const bDisplayMissingContentDialogue =	(bool *)0x00B05BBC;
		volatile char **const sSaveGameSafeCellID =				(volatile char **)0x00B05BB4;
		volatile bool *const bAllowScriptedAutosave =			(bool *)0x00B05BA4;
		volatile bool *const bPreemptivelyUnloadCells =			(bool *)0x00B051CC;
		volatile int *const iNumBitsForFullySeen =				(int *)0x00B0317C;
		volatile int *const iPreloadSizeLimit =					(int *)0x00B030BC;
		volatile char **const sOblivionIntro =					(volatile char **)0x00B030A4;
		volatile bool *const bUseHardDriveCache =				(bool *)0x00B02D60;
		volatile bool *const bEnableBoundingVolumeOcclusion =	(bool *)0x00B02D38;
		volatile bool *const bDisplayBoundingVolumes =			(bool *)0x00B02D30;
	}

	namespace Display {
		volatile float *const fDecalLifetime =					(float *)0x00B097C8;
		volatile bool *const bStaticMenuBackground =			(bool *)0x00B06DC4;
		volatile bool *const bForcePow2Textures =				(bool *)0x00B06DBC;
		volatile bool *const bForce1XShaders =					(bool *)0x00B06DB4;
		volatile bool *const bAllow20HairShader =				(bool *)0x00B06DA4;
		volatile bool *const bAllowScreenShot =					(bool *)0x00B06D14;
		volatile int *const iMultiSample =						(int *)0x00B06D0C;
		volatile bool *const bDoTexturePass =					(bool *)0x00B06CE4;
		volatile bool *const bDoSpecularPass =					(bool *)0x00B06CDC;
		volatile bool *const bDoDiffusePass =					(bool *)0x00B06CD4;
		volatile bool *const bDoAmbientPass =					(bool *)0x00B06CCC;
		volatile bool *const bDoCanopyShadowPass =				(bool *)0x00B06CBC;
		volatile bool *const bDrawShadows =						(bool *)0x00B06CB4;
		volatile bool *const bUseRefractionShader =				(bool *)0x00B06CAC;
		volatile bool *const bUseShaders =						(bool *)0x00B06CA4;
		volatile int *const iLocationY =						(int *)0x00B06C84;
		volatile int *const iLocationX =						(int *)0x00B06C7C;
		volatile bool *const bFullScreen =						(bool *)0x00B06C74;
		volatile int *const iSizeW =							(int *)0x00B06C5C;
		volatile int *const iSizeH =							(int *)0x00B06C64;
		volatile int *const iAdapter =							(int *)0x00B06C54;
		volatile int *const iScreenShotIndex =					(int *)0x00B0316C;
		volatile char **const sScreenShotBaseName =				(volatile char **)0x00B03164;
		volatile int *const iAutoViewMinDistance =				(int *)0x00B0315C;
		volatile int *const iAutoViewHiFrameRate =				(int *)0x00B03154;
		volatile int *const iAutoViewLowFrameRate =				(int *)0x00B0315C;
		volatile bool *const bAutoViewDistance =				(bool *)0x00B03144;
		volatile float *const fDefaultFOV =						(float *)0x00B0313C;
		volatile float *const fNearDistance =					(float *)0x00B03134;	
		volatile int *const iDebugTextLeftRightOffset =			(int *)0x00B02E24;
		volatile int *const iDebugTextTopBottomOffset =			(int *)0x00B02E1C;
		volatile bool *const bShowMenuTextureUse =				(bool *)0x00B02E14;
		volatile int *const iDebugText =						(int *)0x00B02E0C;
		volatile bool *const bLocalMapShader =					(bool *)0x00B06D1C;
		volatile bool *const bDoImageSpaceEffects =				(bool *)0x00B06CC4;
		volatile float *const fShadowLOD2 =						(float *)0x00B06EC4;
		volatile float *const fShadowLOD1 =						(float *)0x00B06EBC;
		volatile float *const fLightLOD2 =						(float *)0x00B06EB4;
		volatile float *const fLightLOD1 =						(float *)0x00B06EAC;
		volatile float *const fSpecularLOD2 =					(float *)0x00B06ED4;
		volatile float *const fSpecularLOD1 =					(float *)0x00B06ECC;
		volatile float *const fEnvMapLOD2 =						(float *)0x00B06EE4;
		volatile float *const fEnvMapLOD1 =						(float *)0x00B06EDC;
		volatile float *const fEyeEnvMapLOD2 =					(float *)0x00B06EF4;
		volatile float *const fEyeEnvMapLOD1 =					(float *)0x00B06EEC;
		volatile int *const iPresentInterval =					(int *)0x00B06F6C;
		volatile float *const fSpecualrStartMax =				(float *)0x00B14894;
		volatile float *const fSpecularStartMin =				(float *)0x00B1488C;
		volatile int *const iActorShadowIntMax =				(int *)0x00B14884;
		volatile int *const iActorShadowIntMin =				(int *)0x00B1487C;
		volatile int *const iActorShadowExtMax =				(int *)0x00B14874;
		volatile int *const iActorShadowExtMin =				(int *)0x00B1486C;
		volatile float *const fGammaMax_Dipslay =				(float *)0x00B14864;
		volatile float *const fGammaMin =						(float *)0x00B1485C;
		volatile int *const iMaxDecalsPerFrame =				(int *)0x00B097D0;
		volatile bool *const bFullBrightLighting =				(bool *)0x00B06F94;
		volatile int *const iMaxLandscapeTextures =				(int *)0x00B06F8C;
		volatile bool *const bDynamicWindowReflections =		(bool *)0x00B06F84;
		volatile float *const fShadowFadeTime =					(float *)0x00B06F7C;
		volatile float *const fGamma =							(float *)0x00B06F64;
		volatile bool *const bDecalsOnSkinnedGeometry =			(bool *)0x00B06F5C;
		volatile int *const iShadowFilter =						(int *)0x00B06F2C;
		volatile int *const iShadowMapResolution =				(int *)0x00B06F1C;
		volatile bool *const bShadowsOnGrass =					(bool *)0x00B06F14;
		volatile bool *const bActorSelfShadowing =				(bool *)0x00B06F0C;
		volatile int *const iActorShadowCountInt =				(int *)0x00B06F04;
		volatile int *const iActorShadowCountExt =				(int *)0x00B06EFC;
		volatile int *const iTexMipMapMinimum =					(int *)0x00B06D2C;
		volatile int *const iTexMipMapSkip =					(int *)0x00B06D24;
		volatile bool *const bDoStaticAndArchShadows =			(bool *)0x00B06CF4;
		volatile bool *const bDoActorShadows =					(bool *)0x00B06CEC;
		volatile float *const fNoLODFarDistancePct =			(float *)0x00B0312C;
		volatile float *const fNoLODFarDistanceMax =			(float *)0x00B03124;
		volatile float *const fNoLODFarDistanceMin =			(float *)0x00B0311C;
		volatile bool *const bLandscapeBlend =					(bool *)0x00B06F9C;
	}
	
	namespace Controls {
		// The key settings are not set to a global static.
		volatile float	*const fVersion =						(float *)0x00B02C4C;
		volatile bool	*const bInvertYValues =					(bool *)0x00B14F38;
		volatile float	*const fXenonLookXYMult =				(float *)0x00B13580;
		volatile float	*const fMouseSensitivity =				(float *)0x00B14EE8;
		volatile int	*const iJoystickMoveFrontBack =			(int *)0x00B14EC8;
		volatile int	*const iJoystickMoveLeftRight =			(int *)0x00B14ED0;
		volatile float	*const fJoystickMoveFBMult =			(float *)0x00B14EF0;
		volatile float	*const fJoystickMoveLRMult =			(float *)0x00B14EF8;
		volatile int	*const iJoystickLookUpDown =			(int *)0x00B14ED8;
		volatile int	*const iJoystickLookLeftRight =			(int *)0x00B14EE0;
		volatile float	*const fJoystickLookUDMult =			(float *)0x00B14F00;
		volatile float	*const fJoystickLookLRMult =			(float *)0x00B14F08;
		volatile bool	*const bBackgroundMouse =				(bool *)0x00B02C3C;
		volatile bool	*const bBackgroundKeyboard =			(bool *)0x00B02C34;
		volatile bool	*const bUseJoystick =					(bool *)0x00B02C2C;
		volatile int	*const iLanguage =						(int *)0x00B02C44;
	}

	namespace Water {
		volatile float 	*const fAlpha =							(float *)0x00B3524C; // ?
		volatile UInt32	*const uSurfaceTextureSize =			(UInt32 *)0x00B35264; // ?
		volatile char	**const sSurfaceTexture =				(volatile char **)0x00B070F0;
		volatile float	*const fNearWaterOutdoorTolerance =		(float *)0x00B070D8;
		volatile float	*const fNearWaterIndoorTolerance =		(float *)0x00B070D0;
		volatile UInt32	*const uNearWaterPoints =				(UInt32 *)0x00B070B8;
		volatile UInt32	*const uNearWaterRadius =				(UInt32 *)0x00B070B0;
		volatile UInt32	*const uSurfaceFrameCount =				(UInt32 *)0x00B07088;
		volatile bool	*const bUseWaterReflectionsMisc =		(bool *)0x00B07080;
		volatile bool	*const bUseWaterReflectionsStatics =	(bool *)0x00B07078;
		volatile bool	*const bUseWaterReflectionsTrees =		(bool *)0x00B07070;
		volatile bool	*const bUseWaterReflectionsActors =		(bool *)0x00B07068;
		volatile bool	*const bUseWaterReflections =			(bool *)0x00B07060;
		volatile bool	*const bUseWaterHiRes =					(bool *)0x00B07058;
		volatile bool	*const bUseWaterDisplacements =			(bool *)0x00B07090;
		volatile bool	*const bUseWaterShader =				(bool *)0x00B07050;
		volatile UInt32	*const uDepthRange =					(UInt32 *)0x00B070E8;
		volatile bool	*const bUseWaterDepth =					(bool *)0x00B070A0;
		volatile bool	*const bUseWaterLOD =					(bool *)0x00B07098;
		volatile float	*const fTileTextureDivisor =			(float *)0x00B07048;
		volatile float	*const fSurfaceTileSize =				(float *)0x00B07040;
		volatile UInt32	*const uNumDepthGrids =					(UInt32 *)0x00B070E0;
	}

	namespace Audio {
		volatile float	*const fMinSoundVel =					(float *)0x00B23C50;
		volatile float	*const fMetalLargeMassMin =				(float *)0x00B162A4;
		volatile float	*const fMetalMediumMassMin =			(float *)0x00B1629C;
		volatile float	*const fStoneLargeMassMin =				(float *)0x00B16264;
		volatile float	*const fStoneMediumMassMin =			(float *)0x00B1625C;
		volatile float	*const fWoodLargeMassMin =				(float *)0x00B16254;
		volatile float	*const fWoodMediumMassMin =				(float *)0x00B1624C;
		volatile float	*const fDialogAttenuationMax =			(float *)0x00B161D0;
		volatile float	*const fDialogAttenuationMin =			(float *)0x00B161C8;
		volatile bool	*const bUseSoundDebugInfo =				(bool *)0x00B161C0;
		volatile float	*const fUnderwaterFrequencyDelta =		(float *)0x00B161B8;
		volatile float	*const fDefaultEffectsVolume =			(float *)0x00B161A8;
		volatile float 	*const fDefaultMusicVolume =			(float *)0x00B161A0;
		volatile float 	*const fDefaultFootVolume =				(float *)0x00B16198;
		volatile float	*const fDefaultVoiceVolume =			(float *)0x00B161B0;
		volatile float	*const fDefaultMasterVolume =			(float *)0x00B16190;
		volatile bool	*const bMusicEnabled =					(bool *)0x00B16180;
		volatile bool	*const bSoundEnabled =					(bool *)0x00B16178;
		volatile float	*const fLargeWeaponWeightMin =			(float *)0x00B162D4;
		volatile float	*const fMediumWeaponWeightMin =			(float *)0x00B162CC;
		volatile float	*const fSkinLargeMassMin =				(float *)0x00B16284;
		volatile float	*const fSkinMediumMassMin =				(float *)0x00B1627C;
		volatile float	*const fDBVoiceAttenuationIn2D =		(float *)0x00B161E0;
		volatile int	*const iCollisionSoundTimeDelta =		(int *)0x00B16244;
		volatile float	*const fClothLargeMassMin =				(float *)0x00B162B4;
		volatile float	*const fClothMediumMassMin =			(float *)0x00B162AC;
		volatile float	*const fEarthLargeMassMin =				(float *)0x00B16274;
		volatile float	*const fEarthMediumMassMin =			(float *)0x00B1626C;
		volatile bool	*const bUseSpeedForWeaponSwish =		(bool *)0x00B162EC;
		volatile float	*const fLargeWeaponSpeedMax =			(float *)0x00B162E4;
		volatile float	*const fMediumWeaponSpeedMax =			(float *)0x00B162DC;
		volatile float	*const fPlayerFootVolume =				(float *)0x00B162F4;
		volatile float	*const fDSoundRolloffFactor =			(float *)0x00B161D8;
		volatile float	*const fMaxFootstepDistance =			(float *)0x00B162FC;
		volatile float	*const fHeadroomdB =					(float *)0x00B23C58;
		volatile int	*const iMaxImpactSoundCount =			(int *)0x00B16304;
		volatile float	*const fMainMenuMusicVolume =			(float *)0x00B16188;
	}

	namespace Pathfinding {
		volatile bool	*const bDrawPathsDefault =				(bool *)0x00B15834;
		volatile bool	*const bPathMovementOnly =				(bool *)0x00B1582C;
		volatile bool	*const bDrawSmoothFailures =			(bool *)0x00B15824;
		volatile bool	*const bDebugSmoothing =				(bool *)0x00B1581C;
		volatile bool	*const bSnapToAngle =					(bool *)0x00B15814;
		volatile bool	*const bDebugAvoidance =				(bool *)0x00B15750;
		volatile bool	*const bUseBackgroundPathing =			(bool *)0x00B15800;
	}

	namespace MAIN {
		// Should be in the combat section.
		volatile float	*const fLowPerfCombatantVoiceDistance =	(float *)0x00B14BAC;
		volatile int	*const iDetectionHighNumPicks =			(int *)0x00B14904;
		// Should be in the General section.
		volatile float	*const fQuestScriptDelayTime =			(float *)0x00B09E28;
	}

	namespace Combat {
		volatile bool	*const bEnableBowZoom_Comabat =			(bool *)0x00B14F48;
		volatile float	*const fMinBloodDamage =				(float *)0x00B148DC;
		volatile float	*const fHitVectorDelay =				(float *)0x00B148D4;
		volatile int	*const iShowHitVector =					(int *)0x00B148CC;
		volatile float	*const fLowPerfNPCTargetLOSTimer =		(float *)0x00B14BC4;
		volatile float	*const fHiPerfNPCTargetLOSTimer =		(float *)0x00B14BBC;
		volatile int	*const iMaxHiPerfNPCTargetCount =		(int *)0x00B14BB4;
		volatile float	*const fLowPerfPCTargetLOSTimer =		(float *)0x00B14BA4;
		volatile float	*const fHiPerfPCTargetLOSTimer =		(float *)0x00B14B9C;
		volatile int	*const iMaxHiPerfPCTargetCount =		(int *)0x00B14B94;
		volatile int	*const iMaxHiPerfCombatCount =			(int *)0x00B148E4;
	}

	namespace HAVOK {
		volatile bool	*const bDisablePlayerCollision =		(bool *)0x00B14F40;
		volatile float	*const fJumpAnimDelay =					(float *)0x00B148EC;
		volatile bool	*const bTreeTops =						(bool *)0x00B1274C;
		volatile int	*const iSimType =						(int *)0x00B11638;
		volatile bool	*const bPreventHavokAddAll =			(bool *)0x00B09870;
		volatile bool	*const bPreventHavokAddClutter =		(bool *)0x00B09868;
		volatile float	*const fMaxTime =						(float *)0x00B097C0;
		volatile bool	*const bHavokDebug =					(bool *)0x00B097B8;
		volatile float	*const fMoveLimitMass =					(float *)0x00B05214;
		volatile int	*const iUpdateType =					(int *)0x00B0520C;
		volatile float	*const fCameraCasterSize =				(float *)0x00B14EC0;
		volatile int	*const iHavokSkipFrameCountTEST =		(int *)0x00B14E44;
		volatile float	*const fHorseRunGravity =				(float *)0x00B14E3C;
		volatile float	*const fQuadrupedPitchMult =			(float *)0x00B14E34;
		volatile int	*const iNumHavokThreads =				(int *)0x00B097D8;
		volatile float	*const fChaseDeltaMult =				(float *)0x00B05234;
		volatile int	*const iEntityBatchRemoveRate =			(int *)0x00B0522C;
		volatile int	*const iMaxPicks =						(int *)0x00B05224;
		volatile bool	*const bAddBipedWhenKeyframed =			(bool *)0x00B0521C;
	}

	namespace Interface {
		volatile float 	*const fDlgLookMult =					(float *)0x00B14F30;
		volatile float	*const fDlgLookAdj =					(float *)0x00B14F28;
		volatile float 	*const fDlgLookDegStop =				(float *)0x00B14F20;
		volatile float	*const fDlgLookDegStart =				(float *)0x00B14F18;
		volatile float	*const fDlgFocus =						(float *)0x00B14F10;
		volatile float	*const fKeyRepeatInterval =				(float *)0x00B135B8;
		volatile float	*const fKeyRepeatTime =					(float *)0x00B135B0;
		volatile float	*const fActivatePickSphereRadius =		(float *)0x00B135A0;
		volatile float	*const fMenuModeAnimBlend =				(float *)0x00B06540;
		volatile int	*const iSafeZoneX =						(int *)0x00B135F8;
		volatile int	*const iSafeZoneY =						(int *)0x00B13600;
		volatile int	*const iSafeZoneXWide =					(int *)0x00B13608;
		volatile int	*const iSafeZoneYWide =					(int *)0x00B13610;
		volatile float	*const fMenuPlayerLightDiffuseBlue =	(float *)0x00B135F0;
		volatile float	*const fMenuPlayerLightDiffuseGreen =	(float *)0x00B135E8;
		volatile float	*const fMenuPlayerLightDiffuseRed =		(float *)0x00B135E0;
		volatile float	*const fMenuPlayerLightAmbientBlue =	(float *)0x00B135D8;
		volatile float	*const fMenuPlayerLightAmbientGreen =	(float *)0x00B135D0;
		volatile float	*const fMenuPlayerLightAmbientRed =		(float *)0x00B135C8;
		volatile bool	*const bActivatePickUseGamebryoPick =	(bool *)0x00B135A8;
		volatile int	*const iMaxViewCasterPicksGamebryo =	(int *)0x00B11920;
		volatile int	*const iMaxViewCasterPicksHavok = 		(int *)0x00B11918;
		volatile int	*const iMaxViewCasterPicksFuzzy =		(int *)0x00B11910;
		volatile bool	*const bUseFuzzyPicking =				(bool *)0x00B11908;
	}

	namespace LoadingBar {
		volatile int	*const iMoveBarWaitingMilliseconds =	(int *)0x00B14170;
		volatile int	*const iMoveBarChaseMilliseconds =		(int *)0x00B14168;
		volatile int	*const iMoveBarMaxMilliseconds =		(int *)0x00B14160;
		volatile float	*const fLoadingSlideDelay =				(float *)0x00B14158;
		volatile float	*const fPercentageOfBar3 =				(float *)0x00B14150;
		volatile float	*const fPercentageOfBar2 =				(float *)0x00B14148;
		volatile float	*const fPercentageOfBar1 =				(float *)0x00B14140;
		volatile float 	*const fPercentageOfBar0 =				(float *)0x00B14138;
		volatile bool	*const bShowSectionTimes =				(bool *)0x00B14130;
	}

	namespace Menu {
		volatile float	*const fCreditsScrollSpeed =			(float *)0x00B13FC4;
		volatile int	*const iConsoleTextYPos =				(int *)0x00B139A4;
		volatile int	*const iConsoleTextXPos =				(int *)0x00B1399C;
		volatile int	*const iConsoleVisibleLines =			(int *)0x00B1398C;
		volatile int	*const iConsoleHistorySize =			(int *)0x00B13984;
		volatile char 	**const rDebugTextColor =				(volatile char **)0x00B12DAC;
		volatile int	*const iConsoleFont =					(int *)0x00B13994;
		volatile int	*const iDebugTextFont =					(int *)0x00B12DB4;
	}

	namespace GamePlay {
		volatile bool	*const bDisableDynamicCrosshair =		(bool *)0x00B13238;
		volatile bool	*const bSaveOnTravel =					(bool *)0x00B13228;
		volatile bool	*const bSaveOnWait =					(bool *)0x00B13220;
		volatile bool	*const bSaveOnRest =					(bool *)0x00B13218;
		volatile bool	*const bCrossHair =						(bool *)0x00B13210;
		volatile bool	*const bGeneralSubtitles =				(bool *)0x00B13208;
		volatile bool	*const bDialogueSubtitles =				(bool *)0x00B13200;
		volatile bool	*const bInstantLevelUp =				(bool *)0x00B14E88;
		volatile bool	*const bHealthBarShowing =				(bool *)0x00B14E90;
		volatile float	*const fHealthBarFadeOutSpeed =			(float *)0x00B14CCC;
		volatile float	*const fHealthBarSpeed =				(float *)0x00B14CC4;
		volatile float	*const fHealthBarHeight =				(float *)0x00B14CBC;
		volatile float	*const fHealthBarWidth =				(float *)0x00B14CB4;
		volatile float	*const fHealthBarEmittanceFadeTime =	(float *)0x00B14CDC;
		volatile float	*const fHealthBarEmittanceTime =		(float *)0x00B14CD4;
		volatile char	**const sTrackLevelUpPath =				(volatile char **)0x00B14EB8;
		volatile float	*const fDifficulty =					(float *)0x00B14EB0;
		volatile bool	*const bTrackLevelUps =					(bool *)0x00B14EA8;
		volatile bool	*const bAllowHavokGrabTheLiving =		(bool *)0x00B14EA0;
		volatile bool	*const bEssentialTakeNoDamage =			(bool *)0x00B14E98;
		volatile int	*const iDetectionPicks =				(int *)0x00B148FC;
		volatile bool	*const bSaveOnInteriorExteriorSwitch =	(bool *)0x00B13230;
	}

	namespace Fonts {
		volatile char	**const sFontFile_1 =					(volatile char **)0x00B12E1C;
		volatile char	**const sFontFile_2 =					(volatile char **)0x00B12E24;
		volatile char	**const sFontFile_3 =					(volatile char **)0x00B12E2C;
		volatile char	**const sFontFile_4 =					(volatile char **)0x00B12E34;
		volatile char	**const sFontFile_5 =					(volatile char **)0x00B12E3C;
	}

	namespace SpeedTree {
		volatile float	*const fCanopyShadowGrassMult =			(float *)0x00B12638;
		volatile int	*const iCanopyShadowScale =				(int *)0x00B12630;
		volatile float	*const fTreeForceMaxBudAngle =			(float *)0x00B12628;
		volatile float	*const fTreeForceMinBudAngle =			(float *)0x00B12620;
		volatile float	*const fTreeForceLeafDimming =			(float *)0x00B12618;
		volatile float	*const fTreeForceBranchDimming =		(float *)0x00B12610;
		volatile float	*const fTreeForceCS =					(float *)0x00B12608;
		volatile bool	*const bEnableTrees =					(bool *)0x00B125E8;
		volatile bool	*const bForceFullLOD =					(bool *)0x00B125F0;
		volatile float	*const fLODTreeMipMapLODBias =			(float *)0x00B02D80;
		volatile float	*const fLocalTreeMipMapLODBias =		(float *)0x00B02D78;
	}

	namespace Debug {
		volatile bool	*const bDebugSaveBuffer =				(bool *)0x00B05BAC;
	}

	namespace BackgroundLoad {
		volatile bool *const bBackgroundLoadLipFiles =						(bool *)0x00B1490C;
		volatile bool *const bUseMultiThreadedFaceGen =						(bool *)0x00B120DC;
		volatile bool *const bBackgroundCellLoads =							(bool *)0x00B08960;
		volatile bool *const bLoadHelmetsInBackground =						(bool *)0x00B06608;
		volatile int *const iAnimationClonePerLoop =						(int *)0x00B06548;
		volatile bool *const bSelectivePurgeUnusedOnFastTravel =			(bool *)0x00B0525C;
		volatile int *const iPostProcessMillisecondsLoadingQueuedPriority =	(int *)0x00B048EC;
		volatile int *const iPostProcessMilliseconds =						(int *)0x00B048E4;
	}

	namespace LOD {
		volatile float	*const fLodDistance =					(float *)0x00B120CC;
		volatile bool	*const bUseFaceGenLOD =					(bool *)0x00B120C4;
		volatile bool	*const bDisplayLODLand =				(bool *)0x00B02D70;
		volatile bool	*const bDisplayLODBuildings =			(bool *)0x00B09AE8;
		volatile bool	*const bDisplayLODTrees =				(bool *)0x00B09AF0;
		volatile bool	*const bLODPopActors =					(bool *)0x00B07644;
		volatile bool	*const bLODPopItems =					(bool *)0x00B0763C;
		volatile bool	*const bLODPopObjects =					(bool *)0x00B07634;
		volatile float	*const fLODFadeOutMultActors =			(float *)0x00B0762C;
		volatile float	*const fLODFadeOutMultItems =			(float *)0x00B07624;
		volatile float	*const fLODFadeOutMultObjects =			(float *)0x00B0761C;
		volatile float	*const fLODMultTrees =					(float *)0x00B0760C;
		volatile int	*const iFadeNodeMinNearDistance =		(int *)0x00B07280;
		volatile float	*const fLODFadeOutPercent =				(float *)0x00B05150;
		volatile float	*const fLODBoundRadiusMult =			(float *)0x00B05148;
		volatile float	*const fTalkingDistance =				(float *)0x00B120D4;
		volatile float	*const fTreeLODMax =					(float *)0x00B1480C;
		volatile float	*const fTreeLODMin =					(float *)0x00B14804;
		volatile float	*const fTreeLODDefault =				(float *)0x00B147FC;
		volatile float	*const fObjectLODMax =					(float *)0x00B14854;
		volatile float	*const fObjectLODMin =					(float *)0x00B1484C;
		volatile float	*const fObjectLODDefault =				(float *)0x00B14844;
		volatile float	*const fItemLODMax =					(float *)0x00B1483C;
		volatile float	*const fItemLODMin =					(float *)0x00B14834;
		volatile float	*const fItemLODDefault =				(float *)0x00B1482C;
		volatile float	*const fActorLODMax =					(float *)0x00B14824;
		volatile float	*const fActorLODMin =					(float *)0x00B1481C;
		volatile float	*const fActorLODDefault =				(float *)0x00B14814;
		volatile bool	*const bForceHideLODLand =				(bool *)0x00B09B48;
		volatile float	*const fLODQuadMinLoadDistance =		(float *)0x00B09AF8;
		volatile float	*const fLODFadeOutActorMultInterior =	(float *)0x00B02DD0;
		volatile float	*const fLODFadeOutItemMultInterior =	(float *)0x00B02DC8;
		volatile float	*const fLODFadeOutObjectMultInterior =	(float *)0x00B02DC0;
		volatile float	*const fLODFadeOutActorMultCity =		(float *)0x00B02D88;
		volatile float	*const fLODFadeOutItemMultCity =		(float *)0x00B02DB0;
		volatile float	*const fLODFadeOutObjectMultCity =		(float *)0x00B02DA8;
		volatile float	*const fLODFadeOutActorMultComplex =	(float *)0x00B02DA0;
		volatile float	*const fLODFadeOutItemMultComplex =		(float *)0x00B02D98;
		volatile float	*const fLODFadeOutObjectMultComplex =	(float *)0x00B02D90;
		volatile float	*const fLODLandVerticalBias =			(float *)0x00B02D68;
	}

	namespace Weather {
		volatile float	*const fSunGlareSize =					(float *)0x00B11E34;
		volatile float	*const fSunBaseSize =					(float *)0x00B11E2C;
		volatile bool	*const bPrecipitation =					(bool *)0x00B11DE4;
	}

	namespace Voice {
		volatile char	**const sFileTypeGame =					(volatile char **)0x00B10D60;
	}

	namespace Grass {
		volatile int	*const iMinGrassSize =					(int *)0x00B09B20;
		volatile float	*const fGrassEndDistance =				(float *)0x00B09B18;
		volatile float	*const fGrassStartFadeDistance =		(float *)0x00B09B10;
		volatile bool	*const bGrassPointLighting =			(bool *)0x00B09B08;
		volatile bool	*const bDrawShaderGrass =				(bool *)0x00B09B00;
		volatile int	*const iGrassDensityEvalSize =			(int *)0x00B08B64;
		volatile int	*const iMaxGrassTypesPerTexure =		(int *)0x00B08B5C;
		volatile float	*const fWaveOffsetRange =				(float *)0x00B080DC;
		volatile float	*const fGrassWindMagnitudeMax =			(float *)0x00B09B30;
		volatile float	*const fGrassWindMagnitudeMin =			(float *)0x00B09B28;
		volatile float	*const fTexturePctThreshold =			(float *)0x00B08B6C;
	}

	namespace Landscape {
		volatile float	*const fLandTextureTilingMult =			(float *)0x00B08B44;
		volatile float	*const fLandFriction =					(float *)0x00B114A4;
		volatile int	*const iLandBorder2B =					(int *)0x00B08B9C;
		volatile int	*const iLandBorder2G =					(int *)0x00B08B94;
		volatile int	*const iLandBorder2R =					(int *)0x00B08B8C;
		volatile int	*const iLandBorder1B =					(int *)0x00B08B84;
		volatile int	*const iLandBorder1G =					(int *)0x00B08B7C;
		volatile int	*const iLandBorder1R =					(int *)0x00B08B74;
	}

	namespace bLightAttenuation{
		volatile float	*const fQuadraticRadiusMult =			(float *)0x00B08190;
		volatile float	*const fLinearRadiusMult =				(float *)0x00B08188;
		volatile bool	*const bOutQuadInLin =					(bool *)0x00B08180;
		volatile float	*const fConstantValue =					(float *)0x00B08178;
		volatile float	*const fQuadraticValue =				(float *)0x00B08170;
		volatile float	*const fLinearValue =					(float *)0x00B08168;
		volatile UInt32	*const uQuadraticMethod =				(UInt32 *)0x00B08160;
		volatile UInt32	*const uLinearMethod =					(UInt32 *)0x00B08158;
		volatile float	*const fFlickerMovement =				(float *)0x00B08150;
		volatile bool	*const bUseQuadratic =					(bool *)0x00B08148;
		volatile bool	*const bUseLinear =						(bool *)0x00B08140;
		volatile bool	*const bUseConstant =					(bool *)0x00B08138;
	}

	namespace BlurShaderHDRInterior {
		volatile float	*const fTargetLUM =						(float *)0x00B06EA4;
		volatile float	*const fUpperLUMClamp =					(float *)0x00B06E9C;
		volatile float	*const fEmissiveHDRMult =				(float *)0x00B06E94;
		volatile float	*const fEyeAdaptSpeed =					(float *)0x00B06E8C;
		volatile float	*const fBrightScale =					(float *)0x00B06E84;
		volatile float	*const fBrightClamp =					(float *)0x00B06E7C;
		volatile float	*const fBlurRadius =					(float *)0x00B06E74;
		volatile int	*const iNumBlurpasses =					(int *)0x00B06E6C;
	}

	namespace BlurShaderHDR {
		volatile float	*const fTargetLUM =						(float *)0x00B06E64;
		volatile float	*const fUpperLUMClamp =					(float *)0x00B06E5C;
		volatile float	*const fGrassDimmer =					(float *)0x00B06E54;
		volatile float	*const fTreeDimmer =					(float *)0x00B06E4C;
		volatile float	*const fEmissiveHDRMult =				(float *)0x00B06E44;
		volatile float	*const fEyeAdaptSpeed =					(float *)0x00B06E3C;
		volatile float	*const fSunlightDimmer =				(float *)0x00B06E34;
		volatile float	*const fSIEmmisiveMult =				(float *)0x00B06E2C;
		volatile float	*const fSISpecularMult =				(float *)0x00B06E24;
		volatile float	*const fSkyBrightness =					(float *)0x00B06E1C;
		volatile float	*const fSunBrightness =					(float *)0x00B06E14;
		volatile float	*const fBrightScale =					(float *)0x00B06E0C;
		volatile float	*const fBrightClamp =					(float *)0x00B06E04;
		volatile float	*const fBlurRadius =					(float *)0x00B06DFC;
		volatile int	*const iNumBlurpasses =					(int *)0x00B06DF4;
		volatile int	*const iBlendType =						(int *)0x00B06DEC;	
		volatile bool	*const bDoHighDynamicRange =			(bool *)0x00B06DE4;
	}

	namespace BlurShader {
		volatile float	*const fSunlightDimmer =				(float *)0x00B06DDC;
		volatile float	*const fSIEmmisiveMult =				(float *)0x00B06D84;
		volatile float	*const fSISpecularMult =				(float *)0x00B06D7C;
		volatile float	*const fSkyBrightness =					(float *)0x00B06D74;
		volatile float 	*const fSunBrightness =					(float *)0x00B06D6C;
		volatile float	*const fAlphaAddExterior =				(float *)0x00B06D64;
		volatile float	*const fAlphaAddInterior =				(float *)0x00B06D5C;
		volatile int	*const iBlurTexSize =					(int *)0x00B06D54;
		volatile float	*const fBlurRadius =					(float *)0x00B06D4C;
		volatile int	*const iNumBlurpasses =					(int *)0x00B06D44;
		volatile int	*const iBlendType =						(int *)0x00B06D3C;
		volatile bool	*const bUseBlurShader =					(bool *)0x00B06D34;
	}

	namespace  GethitShader {
		volatile float	*const fBlurAmmount =					(float *)0x00B06D9C;
		volatile float	*const fBlockedTexOffset =				(float *)0x00B06D94;
		volatile float	*const fHitTexOffset =					(float *)0x00B06D8C;
	}

	namespace MESSAGES {
		volatile bool	*const bBlockMessageBoxes =				(bool *)0x00B06B38;
		volatile bool	*const bDisableWarning =				(bool *)0x00B06B18;
		volatile bool	*const bSkipInitializationFlows =		(bool *)0x00B06B30;
	}

	namespace DistantLOD {
		volatile bool	*const bUseLODLandData =				(bool *)0x00B06AB8;
		volatile float	*const fFadeDistance =					(float *)0x00B06AB0;
		volatile int	*const iDistantLODGroupWidth =			(int *)0x00B2C35C;
	}

	namespace GeneralWarnings {
		volatile char	**const sGeneralMasterMismatchWarning =	(volatile char **)0x00B0557C;
		volatile char	**const sMasterMismatchWarning =		(volatile char **)0x00B05574;
	}

	namespace Archive {
		volatile char	**const sInvalidationFile =				(volatile char **)0x00B04450;
		volatile int	*const iRetainFilenameOffsetTable =		(int *)0x00B04448;
		volatile int	*const iRetainFilenameStringTable =		(int *)0x00B04440;
		volatile int	*const iRetainDirectoryStringTable =	(int *)0x00B04438;
		volatile bool	*const bCheckRuntimeCollisions =		(bool *)0x00B04430;
		volatile bool	*const bInvalidateOlderFiles =			(bool *)0x00B04460;
		volatile bool	*const bUseArchives =					(bool *)0x00B04428;
		volatile char	**const sArchiveList =					(volatile char **)0x00B04458;
	}

	namespace CameraPath {
		volatile int	*const iTake =							(int *)0x00B02D58;
		volatile char	**const sDirectoryName =				(volatile char **)0x00B0309C;
		volatile int	*const iFPS =							(int *)0x00B02D48;
	}

	namespace TestAllCells {
		volatile bool	*const bFileShowTextures =				(bool *)0x00B055BC;
		volatile bool	*const bFileShowIcons =					(bool *)0x00B055B4;
		volatile bool	*const bFileSkipIconChecks =			(bool *)0x00B055AC;
		volatile bool	*const bFileTestLoad =					(bool *)0x00B055A4;
		volatile bool	*const bFileNeededMessage =				(bool *)0x00B0559C;
		volatile bool	*const bFileGoneMessage =				(bool *)0x00B05594;
		volatile bool	*const bFileSkipModelChecks =			(bool *)0x00B05584;
		volatile bool	*const bFileCheckModelCollision =		(bool *)0x00B0558C;
	}

	namespace CopyProtectionStrings {
		volatile char	**const sCopyProtectionMessage2 =		(volatile char **)0x00B02DF0;
		volatile char	**const sCopyProtectionTitle2 =			(volatile char **)0x00B02DE8;
		volatile char	**const SCopyProtectionMessage =		(volatile char **)0x00B02DE0;
		volatile char	**const SCopyProtectionTitle =			(volatile char **)0x00B02DD8;
	}
}