// Fill out your copyright notice in the Description page of Project Settings.


#include "SampleModule.h"


FSampleModule::FSampleModule()
{
}

FSampleModule::~FSampleModule()
{

}

void FSampleModule::Init()
{
    
}

float FSampleModule::GetValue(int32 X, int32 Y)
{
    if(bInited)
    {
        // return Values.GetValue(X, Y, 0);
        return 0.0f;
    }
    else
    {
        UE_LOG(LogHITerrain, Error, TEXT("FSamplenoise::Module::GetValue Not Inited!"))
        return 0.0f;
    }
}


