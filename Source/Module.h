#pragma once

//-----------------------------------------------------------------------------
// Module is a base class for all engine modules (e.g., rendering, input, resources).
// It defines a common interface for initialization, per-frame update, rendering,
// and cleanup. Derived classes should override these virtual methods to implement
// specific module functionality. Modules are managed by the application to
// structure and organize engine subsystems in a modular way.
//-----------------------------------------------------------------------------
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

	virtual void update()
	{
	}

    virtual void preRender()
    {
    }

    virtual void postRender()
	{
	}

    virtual void render()
    {
    }

    virtual bool cleanUp()
	{ 
		return true; 
	}
};
