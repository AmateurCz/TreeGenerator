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
	TArray<FProceduralMeshVertex> caps;
	uint32 pointSize, polySize, polyOffset, pointOffset, capsCount, capsOffset;
};

UCLASS()
class TREEGENERATOR_API ATreeGeneratorActor : public AActor
{
	GENERATED_BODY()

private:
	TArray<Bud*> m_rootBuds;
	TSharedPtr<FRandomStream> rand;
	TSharedPtr<Enviroment> enviroment;
	TArray<AActor*> leafs;

	void ProcessAttractors(Bud* bud);

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
	void GrowBuds(Bud* bud);
	void ProcessResourcesSecondPass(Bud* bud, void* parameters);

	GatherResourcesToRootReturnValue GatherResourcesToRoot(Bud* bud);
	void CalculateBranchModel(Bud* bud, GeometryParameters* parameters);
	void BuildBranchModel(Bud* bud, GeometryParameters* parameters);

	void ATreeGeneratorActor::GenerateModel();

public:

	// Sets default values for this actor's properties
	ATreeGeneratorActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginDestroy() override;

	/***/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Status)
		bool IsGenerating;

	/***/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Status)
		TArray<FString> Obstacles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Status)
		TArray<FString> BoundingVolumes;

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

	/**Number of segments in circle*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Model)
		int32 NumberOfSegments;

	/**Number of iterations during generation*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Model)
		int32 NumberOfIterations;

	// Allow viewing/changing the Material ot the procedural Mesh in editor (if placed in a level at construction)
	UPROPERTY(VisibleAnywhere, Category = Model)
		UProceduralMeshComponent* TreeMesh;

	// Object used as leafs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Materials)
		TSubclassOf<AActor> LeafActor;

	/**Call this to generate tree*/
	UFUNCTION(BlueprintCallable, Category = "Generation")
		void GenerateTree();

};
