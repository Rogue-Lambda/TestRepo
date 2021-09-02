//=========Radiated mutated antlion ============//
//
// Purpose:		Mutated Antlion - very nasty bug
//
//=============================================================================//


#include "cbase.h"
#include "npc_antlion.h"
#include "npc_mutated_antlion.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// base antlion stuff
ConVar	sk_antlionnugget_enabled("sk_antlionnugget_enabled", "1");

enum
{
	NUGGET_NONE,
	NUGGET_SMALL,
	NUGGET_MEDIUM = 1 ,
	NUGGET_LARGE
};

//
//  mutated_antlion nugget
//

class CMutatedNugget : public CItem
{
public:
	DECLARE_CLASS( CMutatedNugget, CItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void Event_Killed( const CTakeDamageInfo &info ); 
	virtual bool VPhysicsIsFlesh( void );
	
	bool	MyTouch( CBasePlayer *pPlayer );
	void	SetDenomination( int nSize ) { Assert( nSize <= NUGGET_LARGE && nSize >= NUGGET_SMALL ); m_nDenomination = nSize; }

	DECLARE_DATADESC();

private:
	int		m_nDenomination;	// Denotes size and health amount given
};

BEGIN_DATADESC( CMutatedNugget )
	DEFINE_FIELD( m_nDenomination, FIELD_INTEGER ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( item_mutatednugget, CMutatedNugget );

void CNPC_Mutated_Antlion::Precache(void)
{
	BaseClass::Precache();

	

	SetModelName(AllocPooledString("models/mutated_antlion.mdl"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Mutated_Antlion::CreateNugget( void )
{
	CMutatedNugget *pNugget = (CMutatedNugget *) CreateEntityByName( "item_mutatednugget" );
	if ( pNugget == NULL )
		return;

	Vector vecOrigin;
	Vector vecForward;
	GetAttachment( LookupAttachment( "glow" ), vecOrigin, &vecForward );

	// Find out what size to make this nugget!
	int nDenomination = GetNuggetDenomination();
	pNugget->SetDenomination( nDenomination );
	
	pNugget->SetAbsOrigin( vecOrigin );
	pNugget->SetAbsAngles( RandomAngle( 0, 360 ) );
	DispatchSpawn( pNugget );

	IPhysicsObject *pPhys = pNugget->VPhysicsGetObject();
	if ( pPhys )
	{
		Vector vecForward;
		GetVectors( &vecForward, NULL, NULL );
		
		Vector vecVelocity = RandomVector( -35.0f, 35.0f ) + ( vecForward * -RandomFloat( 50.0f, 75.0f ) );
		AngularImpulse vecAngImpulse = RandomAngularImpulse( -100.0f, 100.0f );

		pPhys->AddVelocity( &vecVelocity, &vecAngImpulse );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CNPC_Mutated_Antlion::Event_Killed( const CTakeDamageInfo &info )
{
	if ( sk_antlionnugget_enabled.GetBool() )
	{
		CreateNugget();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMutatedNugget::Spawn( void )
{
	Precache();
	
	if ( m_nDenomination == NUGGET_LARGE )
	{
		SetModel( "models/grub_nugget_large.mdl" );
	}
	else if ( m_nDenomination == NUGGET_MEDIUM )
	{
		SetModel( "models/grub_nugget_medium.mdl" );	
	}
	else
	{
		SetModel( "models/grub_nugget_small.mdl" );
	}

	// We're self-illuminating, so we don't take or give shadows
	AddEffects( EF_NOSHADOW|EF_NORECEIVESHADOW );

	m_iHealth = 1;

	BaseClass::Spawn();

	m_takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMutatedNugget::Precache( void )
{
	PrecacheModel("models/grub_nugget_small.mdl");
	PrecacheModel("models/grub_nugget_medium.mdl");
	PrecacheModel("models/grub_nugget_large.mdl");

	PrecacheScriptSound( "GrubNugget.Touch" );
	PrecacheScriptSound( "NPC_Antlion_Grub.Explode" );

	PrecacheParticleSystem( "antlion_spit_player" );
}

//-----------------------------------------------------------------------------
// Purpose: Let us be picked up by the gravity gun, regardless of our material
//-----------------------------------------------------------------------------
bool CMutatedNugget::VPhysicsIsFlesh( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMutatedNugget::MyTouch( CBasePlayer *pPlayer )
{
	//int nHealthToGive = sk_grubnugget_health.GetFloat() * m_nDenomination;
	int nHealthToGive;
	switch (m_nDenomination)
	{
	case NUGGET_SMALL:
		nHealthToGive = sk_grubnugget_health_small.GetInt();
		break;
	case NUGGET_LARGE:
		nHealthToGive = sk_grubnugget_health_large.GetInt();
		break;
	default:
		nHealthToGive = sk_grubnugget_health_medium.GetInt();
	}

	// Attempt to give the player health
	if ( pPlayer->TakeHealth( nHealthToGive, DMG_GENERIC ) == 0 )
		return false;

	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();

	UserMessageBegin( user, "ItemPickup" );
	WRITE_STRING( GetClassname() );
	MessageEnd();

	CPASAttenuationFilter filter( pPlayer, "GrubNugget.Touch" );
	EmitSound( filter, pPlayer->entindex(), "GrubNugget.Touch" );

	UTIL_Remove( this );	

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			*pEvent - 
//-----------------------------------------------------------------------------
void CMutatedNugget::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	int damageType;
	float damage = CalculateDefaultPhysicsDamage( index, pEvent, 1.0f, true, damageType );
	if ( damage > 5.0f )
	{
		CBaseEntity *pHitEntity = pEvent->pEntities[!index];
		if ( pHitEntity == NULL )
		{
			// hit world
			pHitEntity = GetContainingEntity( INDEXENT(0) );
		}
		
		Vector damagePos;
		pEvent->pInternalData->GetContactPoint( damagePos );
		Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
		if ( damageForce == vec3_origin )
		{
			// This can happen if this entity is motion disabled, and can't move.
			// Use the velocity of the entity that hit us instead.
			damageForce = pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
		}

		// FIXME: this doesn't pass in who is responsible if some other entity "caused" this collision
		PhysCallbackDamage( this, CTakeDamageInfo( pHitEntity, pHitEntity, damageForce, damagePos, damage, damageType ), *pEvent, index );
	}

	BaseClass::VPhysicsCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CMutatedNugget::Event_Killed( const CTakeDamageInfo &info )
{
	AddEffects( EF_NODRAW );
	DispatchParticleEffect( "antlion_spit_player", GetAbsOrigin(), QAngle( -90, 0, 0 ) );
	EmitSound( "NPC_Antlion_Grub.Explode" );

	BaseClass::Event_Killed( info );
}