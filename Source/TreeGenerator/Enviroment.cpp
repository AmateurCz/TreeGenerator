// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "Enviroment.h"


Enviroment::Enviroment(TSharedPtr<FRandomStream> randomGenerator, uint32 minAttractors, uint32 maxAttractors, TArray<FBox> boundingVolumes)
{
	m_tiles.Init(0);
	this->m_randomGenerator = randomGenerator;
	this->m_minAttractors = minAttractors;
	this->m_maxAttractors = maxAttractors;
	this->m_boundingVolumes = boundingVolumes;
}


Enviroment::~Enviroment(void)
{
}

void Enviroment::DeactivateAttractorsInSphere(FVector position, double radius){
	for (float x = position.X - radius; x <= position.X + radius; x += ENVIROMENT_TILE_SIZE){
		for (float y = position.Y - radius; y <= position.Y + radius; y += ENVIROMENT_TILE_SIZE){
			for (float z = position.Z - radius; z <= position.Z + radius; z += ENVIROMENT_TILE_SIZE){
				GetTile(position)->DeactivateAttractorsInSphere(position, radius);
			}
		}
	}
}

FVector Enviroment::GetDirectionFromCone(FVector position, FVector axis, double radius, double angle, TArray<Bud*> closeBuds){
	FVector v = FVector(0, 0, 0);
	for (float x = position.X - radius; x <= position.X + radius; x += ENVIROMENT_TILE_SIZE){
		for (float y = position.Y - radius; y <= position.Y + radius; y += ENVIROMENT_TILE_SIZE){
			for (float z = position.Z - radius; z <= position.Z + radius; z += ENVIROMENT_TILE_SIZE){
				EnviromentTile* tile = GetTile(x, y, z);
				if (!tile->IsInitialized())
					tile->Initialize(this->m_randomGenerator, this->m_minAttractors, this->m_maxAttractors, m_boundingVolumes);
				tile = GetTile(x, y, z);
				v += tile->GetDirectionFromCone(position, axis, radius, angle, closeBuds);
			}
		}
	}
	v.Normalize();
	return v;
}

void Enviroment::DeactivateAttractorsInBox(FBox box){
	for (float x = box.Min.X; x <= box.Max.X; x += ENVIROMENT_TILE_SIZE){
		for (float y = box.Min.Y; y <= box.Max.Y; y += ENVIROMENT_TILE_SIZE){
			for (float z = box.Min.Z; z <= box.Max.Z; z += ENVIROMENT_TILE_SIZE){
				GetTile(x,y,z)->DeactivateAttractorsInBox(box);
			}
		}
	}
}

inline int32 GetRoundedPosition(float in){
	return floor(in / ENVIROMENT_TILE_SIZE) * ENVIROMENT_TILE_SIZE;
}

EnviromentTile* Enviroment::GetTile(FVector position){ return GetTile(position.X, position.Y, position.Z); }
EnviromentTile* Enviroment::GetTile(float x, float y, float z){
	int32 begin = 0;
	int32 end = m_tiles.Num();


	int32 value = FMath::Floor(GetRoundedPosition(x)*ENVIROMENT_TILE_SIZE * ENVIROMENT_TILE_SIZE + GetRoundedPosition(y) * ENVIROMENT_TILE_SIZE + GetRoundedPosition(z));

	while (begin < end){
		int index = (end - begin) / 2 + begin;

		int32 tileValue = m_tiles[index]->GetValue();
		if (index == begin)
		{
			if (tileValue == value)
				return m_tiles[index];
			else
				break;
		}

		if (tileValue == value)
			return m_tiles[index];
		if (tileValue < value){
			begin = index;
		}
		else
		{
			end = index;
		}
	}
	EnviromentTile* tile = new EnviromentTile(FVector(GetRoundedPosition(x), GetRoundedPosition(y), GetRoundedPosition(z)));
	tile->Initialize(this->m_randomGenerator, this->m_minAttractors, this->m_maxAttractors, m_boundingVolumes);
	m_tiles.Add(tile);
	m_tiles.Sort();
	return tile;
}