#ifndef _PYGAME_PHYSICS_BODY_
#define _PYGAME_PHYSICS_BODY_


#include "pgVector2.h"
#include "pgDeclare.h"

//! typedef struct _pgWorldObject pgWorldObject;
//! typedef struct _pgShapeObject pgShapeObject;

struct _pgBodyObject{
	PyObject_HEAD

	double		fMass;
	pgVector2	vecLinearVelocity;
	double		fAngleVelocity;
	int			bStatic;

	pgVector2	vecPosition;
	double		fRotation;
	pgVector2	vecImpulse;
	pgVector2	vecForce;
	double		fTorque;

	double		fRestitution;
	double		fFriction;

	pgShapeObject* shape;

	pgVector2 cBiasLV;
	double cBiasW;

};

pgBodyObject* PG_BodyNew();
void	PG_BodyDestroy(pgBodyObject* body);

void PG_FreeUpdateBodyVel(pgWorldObject* world, pgBodyObject* body, double dt);
void PG_FreeUpdateBodyPos(pgBodyObject* body, double dt);
void PG_CorrectBodyPos(pgBodyObject* body, double dt);

//transform point local_p's position from body's local coordinate to the world's global one.
//TODO: is the local coordinate necessary? anyway let it alone right now.
pgVector2 PG_GetGlobalPos(pgBodyObject* body, pgVector2* local_p);

//translate vector from coordinate B to coordinate A
pgVector2 PG_GetRelativePos(pgBodyObject* bodyA, pgBodyObject* bodyB, pgVector2* p_in_B);


//bind rect shape with body
void PG_Bind_RectShape(pgBodyObject* body, double width, double height, double seta);

//get velocity with a local point of a body,assume center is local (0,0)
pgVector2 PG_GetLocalPointVelocity(pgBodyObject* body,pgVector2 localPoint);

#endif //_PYGAME_PHYSICS_BODY_
