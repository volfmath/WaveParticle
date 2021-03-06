#include "WaveParticleCS.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "TextureResource.h"
#include "RHIUtilities.h"
#include "ClearReplacementShaders.h"
#include "Engine/TextureRenderTarget2D.h"


#define WAVE_GROUP_THREAD_COUNTS 8
#define CLEAR_PARTICLE_GROUP_SIZE 8u

class FWaveParticlePlacementCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FWaveParticlePlacementCS, Global)

public:
	FWaveParticlePlacementCS() {};

	FWaveParticlePlacementCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		ThreadWidth.Bind(Initializer.ParameterMap, TEXT("ThreadWidth"));
		ParticleWidth.Bind(Initializer.ParameterMap, TEXT("ParticleWidth"));
		ParticleHeight.Bind(Initializer.ParameterMap, TEXT("ParticleHeight"));
		ParticleNum.Bind(Initializer.ParameterMap, TEXT("ParticleNum"));
		VectorFieldDensity.Bind(Initializer.ParameterMap, TEXT("VectorFieldDensity"));
		ParticleSize.Bind(Initializer.ParameterMap, TEXT("ParticleSize"));
		Beta.Bind(Initializer.ParameterMap, TEXT("Beta"));
		ParticleScale.Bind(Initializer.ParameterMap, TEXT("ParticleScale"));


		WaveParticlePos_SRV.Bind(Initializer.ParameterMap, TEXT("WaveParticlePos"));
		WaveParticleField_UAV.Bind(Initializer.ParameterMap, TEXT("VectorField"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Paramers)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_GROUP_SIZE"), WAVE_GROUP_THREAD_COUNTS);
	}

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const FUpdateFieldStruct& StructData,
		FRHIShaderResourceView* InWaveParticlePos_SRV,
		FRHIUnorderedAccessView* InWaveParticleField_UAV
	)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), ThreadWidth, StructData.InThreadSize.X);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), ParticleWidth, StructData.InParticleQuadSize.X);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), ParticleHeight, StructData.InParticleQuadSize.Y);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), ParticleNum, StructData.InParticleNum);

		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), VectorFieldDensity, StructData.InVectorFieldDensity);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), ParticleSize, StructData.InParticleSize);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), Beta, StructData.InBeta);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundComputeShader(), ParticleScale, StructData.InParticleScale);
		

		SetSRVParameter(RHICmdList, RHICmdList.GetBoundComputeShader(), WaveParticlePos_SRV, InWaveParticlePos_SRV);
		if (WaveParticleField_UAV.IsBound()) {
			RHICmdList.SetUAVParameter(RHICmdList.GetBoundComputeShader(), WaveParticleField_UAV.GetBaseIndex(), InWaveParticleField_UAV);
		}
	}

	void UnbindUAV(FRHICommandList& RHICmdList){

		if (WaveParticleField_UAV.IsBound()) {
			RHICmdList.SetUAVParameter(RHICmdList.GetBoundComputeShader(), WaveParticleField_UAV.GetBaseIndex(), nullptr);
		}
		
	}

	//virtual bool Serialize(FArchive& Ar) override
	//{
	//	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

	//	Ar << ThreadWidth;
	//	Ar << ParticleWidth;
	//	Ar << ParticleHeight;
	//	Ar << VectorFieldDensity;
	//	Ar << ParticleSize;
	//	Ar << Beta;
	//	Ar << ParticleScale;
	//	Ar << ParticleNum;
	//	Ar << WaveParticlePos_SRV;
	//	Ar << WaveParticleField_UAV;
	//	

	//	return bShaderHasOutdatedParameters;
	//}

private:

	LAYOUT_FIELD(FShaderParameter, ThreadWidth);
	LAYOUT_FIELD(FShaderParameter, ParticleWidth);
	LAYOUT_FIELD(FShaderParameter,ParticleHeight);
	LAYOUT_FIELD(FShaderParameter,VectorFieldDensity);
	LAYOUT_FIELD(FShaderParameter,ParticleSize);
	LAYOUT_FIELD(FShaderParameter,Beta);
	LAYOUT_FIELD(FShaderParameter,ParticleScale);
	LAYOUT_FIELD(FShaderParameter,ParticleNum);

	LAYOUT_FIELD(FShaderResourceParameter,WaveParticlePos_SRV);
	LAYOUT_FIELD(FShaderResourceParameter,WaveParticleField_UAV);
};

IMPLEMENT_SHADER_TYPE(, FWaveParticlePlacementCS, TEXT("/Plugins/Shaders/Private/WaveParticleCS.usf"), TEXT("ComputeFieldCS"), SF_Compute)


class FWaveParticleClearCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FWaveParticleClearCS, Global)

public:
	FWaveParticleClearCS() {};

	FWaveParticleClearCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		ClearTarget.Bind(Initializer.ParameterMap, TEXT("VectorField"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Paramers)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("CLEAR_THREAD_GROUP_SIZE"), CLEAR_PARTICLE_GROUP_SIZE);
	}

	//virtual bool Serialize(FArchive& Ar) override
	//{
	//	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

	//	Ar << ClearTarget;

	//	return bShaderHasOutdatedParameters;
	//}

	uint32 GetResourceParamIndex() { return ClearTarget.GetBaseIndex(); }

public:
	LAYOUT_FIELD(FShaderResourceParameter,ClearTarget);
};


IMPLEMENT_SHADER_TYPE(, FWaveParticleClearCS, TEXT("/Plugins/Shaders/Private/WaveParticleClearCS.usf"), TEXT("ClearComputeFieldCS"), SF_Compute);


class FWaveParticleCompressionCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FWaveParticleCompressionCS, Global)

public:
	FWaveParticleCompressionCS() {};

	FWaveParticleCompressionCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		VectorFieldSize.Bind(Initializer.ParameterMap, TEXT("VectorFieldSize"));
		SINTVectorFieldTex.Bind(Initializer.ParameterMap, TEXT("VectorField"));
		CompressionTarget.Bind(Initializer.ParameterMap, TEXT("CompressionTarget"));
		NormalTarget.Bind(Initializer.ParameterMap, TEXT("NormalTarget"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Paramers)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("COMPORESSION_THREAD_GROUP_SIZE"), WAVE_GROUP_THREAD_COUNTS);
	}

	//virtual bool Serialize(FArchive& Ar) override
	//{
	//	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

	//	Ar << VectorFieldSize;
	//	Ar << SINTVectorFieldTex;
	//	Ar << CompressionTarget;
	//	Ar << NormalTarget;

	//	return bShaderHasOutdatedParameters;
	//}


public:
	//OpenGL es not support read/write same texture
	LAYOUT_FIELD(FShaderParameter,VectorFieldSize);

	LAYOUT_FIELD(FShaderResourceParameter,SINTVectorFieldTex);
	LAYOUT_FIELD(FShaderResourceParameter,CompressionTarget);
	LAYOUT_FIELD(FShaderResourceParameter,NormalTarget);
};


IMPLEMENT_SHADER_TYPE(, FWaveParticleCompressionCS, TEXT("/Plugins/Shaders/Private/WaveParticleCompression.usf"), TEXT("CompressionFieldCS"), SF_Compute);


FWaveParticle_GPU::FWaveParticle_GPU(
	const TSharedPtr<TArray<FVector2D>, ESPMode::ThreadSafe>& SharedParticlePos, 
	const TSharedPtr<TArray<FVector2D>, ESPMode::ThreadSafe>& SharedParticleSpeed
)
	:
	SharedParticlePos(SharedParticlePos),
	SharedParticleSpeed(SharedParticleSpeed)
{
	WaveParticlePosBuffer = new FReadBuffer();
	WaveParticleFieldBuffer = new FTextureRWBuffer2D();
	WaveParticleFieldComPression = new FTextureRWBuffer2D();
	WaveParticleNormal = new FTextureRWBuffer2D();
}


FWaveParticle_GPU::~FWaveParticle_GPU() {
	delete WaveParticlePosBuffer;
	delete WaveParticleFieldBuffer;
	delete WaveParticleFieldComPression;
	delete WaveParticleNormal;

	WaveParticlePosBuffer = nullptr;
	WaveParticleFieldBuffer = nullptr;
	WaveParticleFieldComPression = nullptr;
	WaveParticleNormal = nullptr;
}

void FWaveParticle_GPU::UpdateWaveParticlePos(float ParticleTickTime)
{
	for (auto i = 0; i < SharedParticlePos->Num(); ++i) {
		(*SharedParticlePos)[i] += ParticleTickTime * (*SharedParticleSpeed)[i];
	}
	void* ParticlePosData = RHILockVertexBuffer(WaveParticlePosBuffer->Buffer, 0, SharedParticlePos->Num() * sizeof(FVector2D), RLM_WriteOnly);
	FMemory::Memcpy(ParticlePosData, SharedParticlePos->GetData(), SharedParticlePos->Num() * sizeof(FVector2D));
	RHIUnlockVertexBuffer(WaveParticlePosBuffer->Buffer);
}


void FWaveParticle_GPU::UpdateWaveParticleFiled(FRHICommandListImmediate& RHICmdList,const FUpdateFieldStruct& StructData,ERHIFeatureLevel::Type FeatureLevel, UTextureRenderTarget2D* CopyVectorFieldTexPtr, UTextureRenderTarget2D* CopyNormal)
{
	RHICmdList.BeginComputePass(TEXT("ComputeField"));
	//Clear Pass
	{
		TShaderMapRef<FWaveParticleClearCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = ComputeShader.GetComputeShader();
		RHICmdList.SetComputeShader(ShaderRHI);
		SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->ClearTarget, WaveParticleFieldBuffer->UAV);
		DispatchComputeShader(
			RHICmdList,
			ComputeShader,
			FMath::DivideAndRoundUp(WaveParticleFieldBuffer->Buffer->GetSizeX(), CLEAR_PARTICLE_GROUP_SIZE),
			FMath::DivideAndRoundUp(WaveParticleFieldBuffer->Buffer->GetSizeY(), CLEAR_PARTICLE_GROUP_SIZE),
			1
		);
		SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->ClearTarget, nullptr);
		RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, WaveParticleFieldBuffer->UAV);
	}


	//calculate VectorField
	{
		TShaderMapRef<FWaveParticlePlacementCS> WaveParticlePlacementShader(GetGlobalShaderMap(FeatureLevel));
		RHICmdList.SetComputeShader(WaveParticlePlacementShader.GetComputeShader());
		WaveParticlePlacementShader->SetParameters(
			RHICmdList,
			StructData,
			WaveParticlePosBuffer->SRV,
			WaveParticleFieldBuffer->UAV
		);

		uint32 GroupSize_X = StructData.InThreadSize.X / CLEAR_PARTICLE_GROUP_SIZE;
		uint32 GroupSize_Y = StructData.InThreadSize.Y / CLEAR_PARTICLE_GROUP_SIZE;

		DispatchComputeShader(
			RHICmdList,
			WaveParticlePlacementShader,
			GroupSize_X,
			GroupSize_Y,
			1ul
		);

		WaveParticlePlacementShader->UnbindUAV(RHICmdList);

		RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, WaveParticleFieldBuffer->UAV, nullptr);
	}


	//Comporession VectorField and calculate normal
	{
		TShaderMapRef<FWaveParticleCompressionCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = ComputeShader.GetComputeShader();
		RHICmdList.SetComputeShader(ShaderRHI);
		SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->VectorFieldSize, StructData.InVectorFieldSize);
		SetTextureParameter(RHICmdList, ShaderRHI, ComputeShader->SINTVectorFieldTex, WaveParticleFieldBuffer->Buffer);
		SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->CompressionTarget, WaveParticleFieldComPression->UAV);
		SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->NormalTarget, WaveParticleNormal->UAV);
		RHICmdList.DispatchComputeShader(
			FMath::DivideAndRoundUp(WaveParticleFieldComPression->Buffer->GetSizeX(), CLEAR_PARTICLE_GROUP_SIZE),
			FMath::DivideAndRoundUp(WaveParticleFieldComPression->Buffer->GetSizeY(), CLEAR_PARTICLE_GROUP_SIZE),
			1
		);
		SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->CompressionTarget, nullptr);
		SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->NormalTarget, nullptr);
		RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToGfx, WaveParticleFieldComPression->UAV);
		RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToGfx, WaveParticleNormal->UAV);
	}
	RHICmdList.EndComputePass();


	//#TODO: 可修改UTexture2D来避免复制，在InitRHI()时带上对应标志

	FTexture2DRHIRef MaterialFieldTextureRHI = static_cast<FTextureRenderTarget2DResource*>(CopyVectorFieldTexPtr->Resource)->GetTextureRHI();
	FTexture2DRHIRef MaterialNormalTextureRHI = static_cast<FTextureRenderTarget2DResource*>(CopyNormal->Resource)->GetTextureRHI();
	RHICmdList.CopyTexture(WaveParticleFieldComPression->Buffer, MaterialFieldTextureRHI, FRHICopyTextureInfo());
	RHICmdList.CopyTexture(WaveParticleNormal->Buffer, MaterialNormalTextureRHI, FRHICopyTextureInfo());
}


void FWaveParticle_GPU::InitWaveParticlePosResource(const FIntPoint& FieldSize) {
	WaveParticlePosBuffer->Initialize(sizeof(FVector2D), SharedParticlePos->Num(), EPixelFormat::PF_G32R32F, BUF_Dynamic);
	WaveParticleFieldBuffer->Initialize(sizeof(uint32), FieldSize.X * 3, FieldSize.Y, EPixelFormat::PF_R32_SINT);

	//WaveParticleFieldComPression->Initialize(sizeof(float) * 4, FieldSize.X, FieldSize.Y, EPixelFormat::PF_A32B32G32R32F);
	//WaveParticleNormal->Initialize(sizeof(float) * 4, FieldSize.X, FieldSize.Y, EPixelFormat::PF_A32B32G32R32F);


	////VectorField压缩到16f
	WaveParticleFieldComPression->Initialize(sizeof(float) * 2, FieldSize.X, FieldSize.Y, EPixelFormat::PF_FloatRGBA);
	WaveParticleNormal->Initialize(sizeof(float) * 2, FieldSize.X, FieldSize.Y, EPixelFormat::PF_FloatRGBA);
}