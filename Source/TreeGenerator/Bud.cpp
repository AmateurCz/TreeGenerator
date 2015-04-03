// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "Bud.h"


Bud::Bud(FVector position, FVector axis) {
	this->allignedBud = NULL;
	this->secondaryAllignedBud = NULL;
	this->parentBud = NULL;
	axis.Normalize();
	this->axis = this->originalAxis = axis;
	this->position = position;
	this->resource = 0;
	this->weight = 0;
	this->diameter = 0;
	this->lenght = 0;
	this->isDead = false;
}
Bud::Bud(FVector position, FVector axis, Bud* parrent) : Bud(position, axis){
	if (parrent->allignedBud){
		parrent->secondaryAllignedBud = this;
	}
	else{
		parrent->allignedBud = this;
	}
	this->parentBud = parrent;
}
Bud::~Bud(){
	if (this->allignedBud)
		delete this->allignedBud;
	if (this->secondaryAllignedBud)
		delete this->secondaryAllignedBud;
	if (this->parentBud){
		if (this->parentBud->allignedBud == this){
			this->parentBud->allignedBud = this->parentBud->secondaryAllignedBud;
			this->parentBud->secondaryAllignedBud = NULL;
		}
		if (this->parentBud->secondaryAllignedBud == this)
			this->parentBud->secondaryAllignedBud = NULL;
	}
}