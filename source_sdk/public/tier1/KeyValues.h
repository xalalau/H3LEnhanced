//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef KEYVALUES_H
#define KEYVALUES_H

#ifdef _WIN32
#pragma once
#endif

// #include <vgui/VGUI.h>

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#include "UtlVector.h"
#include "SDK_Color.h"

class IFileSystem;
class CUtlBuffer;
class SDK_Color;
typedef void * FileHandle_t;

//-----------------------------------------------------------------------------
// Purpose: Simple recursive data access class
//			Used in vgui for message parameters and resource files
//			Destructor deletes all child KeyValues nodes
//			Data is stored in key (string names) - (string/int/float)value pairs called nodes.
//
//	About KeyValues Text File Format:

//	It has 3 control characters '{', '}' and '"'. Names and values may be quoted or
//	not. The quote '"' charater must not be used within name or values, only for
//	quoting whole tokens. You may use escape sequences wile parsing and add within a
//	quoted token a \" to add quotes within your name or token. When using Escape
//	Sequence the parser must now that by setting KeyValues::UsesEscapeSequences( true ),
//	which it's off by default. Non-quoted tokens ends with a whitespace, '{', '}' and '"'.
//	So you may use '{' and '}' within quoted tokens, but not for non-quoted tokens.
//  An open bracket '{' after a key name indicates a list of subkeys which is finished
//  with a closing bracket '}'. Subkeys use the same definitions recursively.
//  Whitespaces are space, return, newline and tabulator. Allowed Escape sequences
//	are \n, \t, \\, \n and \". The number character '#' is used for macro purposes 
//	(eg #include), don't use it as first charater in key names.
//-----------------------------------------------------------------------------
class KeyValues
{
public:
	KeyValues( const char *setName );

	// Quick setup constructors
	KeyValues( const char *setName, const char *firstKey, const char *firstValue );
	KeyValues( const char *setName, const char *firstKey, const wchar_t *firstValue );
	KeyValues( const char *setName, const char *firstKey, int firstValue );
	KeyValues( const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue );
	KeyValues( const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue );

	// Section name
	virtual const char *GetName() const;
	void SetName( const char *setName);

	// gets the name as a unique int
	virtual int GetNameSymbol() const;

	// File access. Set UsesEscapeSequences true, if resource file/buffer uses Escape Sequences (eg \n, \t)
	void UsesEscapeSequences(bool state); // default false
	virtual bool LoadFromFile( IFileSystem *filesystem, const char *resourceName, const char *pathID = NULL );
	virtual bool SaveToFile( IFileSystem *filesystem, const char *resourceName, const char *pathID = NULL);

	// Read from a buffer...  Note that the buffer must be null terminated
	bool LoadFromBuffer( char const *resourceName, const char *pBuffer, IFileSystem* pFileSystem = NULL, const char *pPathID = NULL );

	// Read from a utlbuffer...
	bool LoadFromBuffer( char const *resourceName, CUtlBuffer &buf, IFileSystem* pFileSystem = NULL, const char *pPathID = NULL );

	// Find a keyValue, create it if it is not found.
	// Set bCreate to true to create the key if it doesn't already exist (which ensures a valid pointer will be returned)
	virtual KeyValues *FindKey(const char *keyName, bool bCreate = false);
	virtual KeyValues *FindKey(int keySymbol) const;
	virtual KeyValues *CreateNewKey();		// creates a new key, with an autogenerated name.  name is guaranteed to be an integer, of value 1 higher than the highest other integer key name
	void AddSubKey( KeyValues *pSubkey );	// Adds a subkey. Make sure the subkey isn't a child of some other keyvalues
	virtual void RemoveSubKey(KeyValues *subKey);	// removes a subkey from the list, DOES NOT DELETE IT

	// Key iteration.
	//
	// NOTE: GetFirstSubKey/GetNextKey will iterate keys AND values. Use the functions 
	// below if you want to iterate over just the keys or just the values.
	//
	virtual KeyValues *GetFirstSubKey();	// returns the first subkey in the list
	virtual KeyValues *GetNextKey();		// returns the next subkey
	void SetNextKey( KeyValues * pDat);

	//
	// These functions can be used to treat it like a true key/values tree instead of 
	// confusing values with keys.
	//
	// So if you wanted to iterate all subkeys, then all values, it would look like this:
	//     for ( KeyValues *pKey = pRoot->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	//     {
	//		   Msg( "Key name: %s\n", pKey->GetName() );
	//     }
	//     for ( KeyValues *pValue = pRoot->GetFirstValue(); pKey; pKey = pKey->GetNextValue() )
	//     {
	//         Msg( "Int value: %d\n", pValue->GetInt() );  // Assuming pValue->GetDataType() == TYPE_INT...
	//     }
	KeyValues* GetFirstTrueSubKey();
	KeyValues* GetNextTrueSubKey();

	KeyValues* GetFirstValue();	// When you get a value back, you can use GetX and pass in NULL to get the value.
	KeyValues* GetNextValue();


	// Data access
	virtual int   GetInt( const char *keyName = NULL, int defaultValue = 0 );
	uint64 GetUint64( const char *keyName = NULL, uint64 defaultValue = 0 );
	virtual float GetFloat( const char *keyName = NULL, float defaultValue = 0.0f );
	virtual const char *GetString( const char *keyName = NULL, const char *defaultValue = "" );
	virtual const wchar_t *GetWString( const char *keyName = NULL, const wchar_t *defaultValue = L"" );
	virtual void *GetPtr( const char *keyName = NULL, void *defaultValue = (void*)0 );
	SDK_Color GetColor( const char *keyName = NULL /* default value is all black */);
	virtual bool  IsEmpty(const char *keyName = NULL);

	// Data access
	int   GetInt( int keySymbol, int defaultValue = 0 );
	float GetFloat( int keySymbol, float defaultValue = 0.0f );
	const char *GetString( int keySymbol, const char *defaultValue = "" );
	const wchar_t *GetWString( int keySymbol, const wchar_t *defaultValue = L"" );
	void *GetPtr( int keySymbol, void *defaultValue = (void*)0 );
	SDK_Color GetColor( int keySymbol /* default value is all black */);
	bool  IsEmpty( int keySymbol );

	// Key writing
	virtual void SetWString( const char *keyName, const wchar_t *value );
	virtual void SetString( const char *keyName, const char *value );
	virtual void SetInt( const char *keyName, int value );
	void SetUint64( const char *keyName, uint64 value );
	virtual void SetFloat( const char *keyName, float value );
	virtual void SetPtr( const char *keyName, void *value );
	void SetColor( const char *keyName, SDK_Color value);

	// Memory allocation (optimized)
	void *operator new( size_t iAllocSize );
	void *operator new( size_t iAllocSize, int nBlockUse, const char *pFileName, int nLine );
	void operator delete( void *pMem );
	void operator delete( void *pMem, int nBlockUse, const char *pFileName, int nLine );

	KeyValues& operator=( KeyValues& src );

	// Adds a chain... if we don't find stuff in this keyvalue, we'll look
	// in the one we're chained to.
	void ChainKeyValue( KeyValues* pChain );
	
	void RecursiveSaveToFile( CUtlBuffer& buf, int indentLevel );

	bool WriteAsBinary( CUtlBuffer &buffer );
	bool ReadAsBinary( CUtlBuffer &buffer );

	// Allocate & create a new copy of the keys
	virtual KeyValues *MakeCopy( void ) const;

	// Make a new copy of all subkeys, add them all to the passed-in keyvalues
	void CopySubkeys( KeyValues *pParent ) const;

	// Clear out all subkeys, and the current value
	virtual void Clear( void );

	// Data type
	enum types_t
	{
		TYPE_NONE = 0,
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_PTR,
		TYPE_WSTRING,
		TYPE_COLOR,
		TYPE_UINT64,
		TYPE_NUMTYPES, 
	};
	virtual types_t GetDataType(const char *keyName = NULL);

	// Virtual deletion function - ensures that KeyValues object is deleted from correct heap
	virtual void deleteThis();

	void		SetStringValue( char const *strValue );

private:
	KeyValues( KeyValues& );	// prevent copy constructor being used

	// prevent delete being called except through deleteThis()
	~KeyValues();

	KeyValues* CreateKey( const char *keyName );
	
	void RecursiveCopyKeyValues( KeyValues& src );
	void RemoveEverything();
//	void RecursiveSaveToFile( IBaseFileSystem *filesystem, CUtlBuffer &buffer, int indentLevel );
//	void WriteConvertedString( CUtlBuffer &buffer, const char *pszString );
	
	// NOTE: If both filesystem and pBuf are non-null, it'll save to both of them.
	// If filesystem is null, it'll ignore f.
	void RecursiveSaveToFile( IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel );
	void WriteConvertedString( IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const char *pszString );
	
	void RecursiveLoadFromBuffer( char const *resourceName, CUtlBuffer &buf );

	// For handling #include "filename"
	void AppendIncludedKeys( CUtlVector< KeyValues * >& includedKeys );
	void ParseIncludedKeys( char const *resourceName, const char *filetoinclude, 
							IFileSystem* pFileSystem, const char *pPathID, CUtlVector< KeyValues * >& includedKeys );

	// NOTE: If both filesystem and pBuf are non-null, it'll save to both of them.
	// If filesystem is null, it'll ignore f.
	void InternalWrite( IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const void *pData, int len );
	
	void Init();
	const char * ReadToken( CUtlBuffer &buf, bool &wasQuoted );
	void WriteIndents( IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel );

	void FreeAllocatedValue();
	void AllocateValueBlock(int size);

	int m_iKeyName;	// keyname is a symbol defined in KeyValuesSystem

	// These are needed out of the union because the API returns string pointers
	char *m_sValue;
	wchar_t *m_wsValue;

	// we don't delete these
	union
	{
		int m_iValue;
		float m_flValue;
		void *m_pValue;
		unsigned char m_Color[4];
	};
	
#ifdef _XBOX
	char	   m_iDataType;
	char	   m_bHasEscapeSequences; // true, if while parsing this KeyValue, Escape Sequences are used (default false)
	char	   reserved[2];

	KeyValues *m_pPeer;	// pointer to next key in list
	KeyValues *m_pSub;	// pointer to Start of a new sub key list
	KeyValues *m_pChain;// Search here if it's not in our list
#else
	char	   m_iDataType;
	char	   reserved[5];

	KeyValues *m_pPeer;	// pointer to next key in list
	KeyValues *m_pSub;	// pointer to Start of a new sub key list
	KeyValues *m_pChain;// Search here if it's not in our list
	char	   m_bHasEscapeSequences; // true, if while parsing this KeyValue, Escape Sequences are used (default false)
#endif
};


//-----------------------------------------------------------------------------
// inline methods
//-----------------------------------------------------------------------------
inline int   KeyValues::GetInt( int keySymbol, int defaultValue )
{
	KeyValues *dat = FindKey( keySymbol );
	return dat ? dat->GetInt( (const char *)NULL, defaultValue ) : defaultValue;
}

inline float KeyValues::GetFloat( int keySymbol, float defaultValue )
{
	KeyValues *dat = FindKey( keySymbol );
	return dat ? dat->GetFloat( (const char *)NULL, defaultValue ) : defaultValue;
}

inline const char *KeyValues::GetString( int keySymbol, const char *defaultValue )
{
	KeyValues *dat = FindKey( keySymbol );
	return dat ? dat->GetString( (const char *)NULL, defaultValue ) : defaultValue;
}

inline const wchar_t *KeyValues::GetWString( int keySymbol, const wchar_t *defaultValue )
{
	KeyValues *dat = FindKey( keySymbol );
	return dat ? dat->GetWString( (const char *)NULL, defaultValue ) : defaultValue;
}

inline void *KeyValues::GetPtr( int keySymbol, void *defaultValue )
{
	KeyValues *dat = FindKey( keySymbol );
	return dat ? dat->GetPtr( (const char *)NULL, defaultValue ) : defaultValue;
}

inline SDK_Color KeyValues::GetColor( int keySymbol )
{
	SDK_Color defaultValue( 0, 0, 0, 0 );
	KeyValues *dat = FindKey( keySymbol );
	return dat ? dat->GetColor( ) : defaultValue;
}

inline bool  KeyValues::IsEmpty( int keySymbol )
{
	KeyValues *dat = FindKey( keySymbol );
	return dat ? dat->IsEmpty( ) : true;
}


#endif // KEYVALUES_H
