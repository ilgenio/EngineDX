#pragma once

#include "Globals.h"

class Module
{
public:

	Module()
	{
	}

    virtual ~Module()
    {

    }

	virtual bool init() 
	{
		return true; 
	}

	virtual UpdateStatus preUpdate()
	{
		return UPDATE_CONTINUE;
	}

	virtual UpdateStatus update()
	{
		return UPDATE_CONTINUE;
	}

	virtual UpdateStatus postUpdate()
	{
		return UPDATE_CONTINUE;
	}

	virtual bool cleanUp() 
	{ 
		return true; 
	}
};
