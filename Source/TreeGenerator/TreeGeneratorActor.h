// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Bud.h"
#include "Enviroment.h"
#include "ProceduralMeshComponent.h"
#include "TreeGeneratorActor.generated.h"


struct GatherResourcesToRootReturnValue{
	double resources;
	uint32 weight;
};

struct PolygonIndices{
	int32 A, B, C, D;
};
struct GeometryParameters
{
	TArray<FProceduralMeshVertex> points;
	TArray<PolygonIndices> polys;
	uint32 pointSize, polySize, polyOffset, pointOffset, capsCount;
};

UCLASS()
class TREEGENERATOR_API ATreeGeneratorActor : public AActor, public FRunnable
{
	GENERATED_BODY()

private:
	TArray<Bud*> m_rootBuds;
	TSharedPtr<FRandomStream> rand;
	TSharedPtr<Enviroment> enviroment;
	TArray<UStaticMeshComponent*> leafs;

	void ProcessAttractors(Bud* bud);

	FRunnableThread* workerThread;
	bool workerHasFinished;
	TArray<FBox> boundingVol;
	TArray<FBox> obstacles;

	//double lightSthrenght, gravityStrenght, environmentStrength;

	typedef void (ATreeGeneratorActor::*BudFunction)(Bud*, void*);

	void  UpdateDiameter(Bud* bud, double branchDiameter);

	void ProcessBud(Bud* bud, void* parameters, BudFunction);
	void ProcessAttractors(Bud* bud, void* parameters);
	void KillBud(Bud* bud, void* parameters);
	void ProcessAttractorsConical(Bud* bud, void* parameters);
	void ProcessShadowMap(Bud* bud, void* parameters);
	void ProcessResourcesFirstPass(Bud* bud, void* parameters);
	void CalcShadow(Bud* bud, void* parameters);
	void GetBudsInRadius(Bud* bud, void* parameters);
	void GrowBuds(Bud* bud, TArray<FBox> obstacles);
	void ProcessResourcesSecondPass(Bud* bud, void* parameters);

	GatherResourcesToRootReturnValue GatherResourcesToRoot(Bud* bud);
	void CalculateBranchModel(Bud* bud, GeometryParameters* parameters);
	void BuildBranchModel(Bud* bud, GeometryParameters* parameters);

	void SpawnLeaf(FVector position, FVector normal);
public:

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	// End FRunnable interface

	// Sets default values for this actor's properties
	ATreeGeneratorActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginDestroy() override;

	/**Sets radius ocupied by bud*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enviroment)
		float OcupiedRadius;

	/**Sets radius of enviroment influence*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enviroment)
		float EnviromentConeRadius;

	/**Sets angle of enviroment influence*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enviroment)
		float EnviromentConeAngle;

	/**Determines added length of branch on each iteration*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enviroment)
		float EnviromentStrength;

	/**Determines force pulling branches toward ground*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enviroment)
		float GravityStrength;

	/**Determines how fast will obscure branches die*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enviroment)
		float LightStrength;

	/**Sets minimum of attractors in tile*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enviroment)
		int32 AttractorsMinimum;

	/**Sets maximum of attractors in tile*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enviroment)
		int32 AttractorsMaximum;

	/**Array of roots*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enviroment)
		TArray<FVector> Roots;

	/**Diameter at tip of the branch*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Model)
		float BranchDiameter;

	/***/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Model)
		float BranchDiameterExponent;

	/**Number of segments in circle*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Model)
		int32 NumberOfSegments;

	/**Number of iterations during generation*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Model)
		int32 NumberOfIterations;

	/**Material used on tree mesh*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Model)
		UMaterialInterface* Material;

	// Allow viewing/changing the Material ot the procedural Mesh in editor (if placed in a level at construction)
	UPROPERTY(VisibleAnywhere, Category = Model)
		UProceduralMeshComponent* TreeMesh;

	// Object used as leafs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Leafs)
		FVector2D AngleX;

	// Object used as leafs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Leafs)
		FVector2D AngleY;

	// Object used as leafs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Leafs)
		FVector2D AngleZ;

	// Object used as leafs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Leafs)
		float GenerateLeafsUpToDiameter;

	// Object used as leafs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Leafs)
		TArray<UStaticMesh*> LeafMeshes;

	/**Call this to generate tree and model*/
	UFUNCTION(BlueprintCallable, Category = "Generation")
		void GenerateTree();

	/**Call this to generate model*/
	UFUNCTION(BlueprintCallable, Category = "Generation")
		void GenerateModel();
	/***/
	UFUNCTION(BlueprintCallable, Category = Status)
		bool IsGenerating();

};
