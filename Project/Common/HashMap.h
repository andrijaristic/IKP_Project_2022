#pragma once

#ifndef HASHMAP_H
#define HASHMAP_H

#include <iostream>
#include <cstdlib>
#include <Windows.h>
#define DEFAULT_SIZE 100
using namespace std;

template <class T>
class HashMapItem
{
private:
	char* key;
	T value;
	HashMapItem* next;
	template <class T> friend class HashMap;

public:
	HashMapItem(char* key, T value);		//Constructor
	HashMapItem();							//Constructor
	~HashMapItem();							//Destructor

};

//Hash map that maps char* strings to generic values
template <class T>
class HashMap
{
private:
	HashMapItem<T>* items;					// Array of items that are stored in the map
	CRITICAL_SECTION MapAccess;				// Critical secion for thread safety
	unsigned int size;						// Array size (maximum number of unique keys in the map)
	unsigned int count;						// Current number of items in the map
	long long GetHash(const char* str);		// Hash function

public:
	HashMap(unsigned int size = DEFAULT_SIZE);		//Constructor
	~HashMap();											//Destructor
	void Insert(const char* key, T value);					// Inserts value into map, if key exists, replaces the value
	void Delete(const char* key);								// Deletes item with given key from map and returns true, if key doesnt exists returns false
	bool Get(const char* key, T* value);						// Tries getting the value and stores it into value parameter, returs true if it succeeds, or false if key doesnt exist
	bool ContainsKey(const char* key);						// Checks if given key exists in the map
	char** GetKeys(int* keyCount);							// Returns all keys from hash map, to avoid memory leakage, memory for each key, as well as memory for array of keys must be freed
	T* GetValues(int* valuesCount);						    // Returns all values from hash map, to avoid memory leakage memory for array of values must be freed
};

template <class T>
HashMapItem<T>::HashMapItem(char* key, T value)
{
	this->key = _strdup(key);
	this->value = value;
	this->next = NULL;
}

template <class T>
HashMapItem<T>::HashMapItem()
{
	this->key = NULL;
	this->next = NULL;
}

template <class T>
HashMapItem<T>::~HashMapItem()
{
	if (this->key != NULL)
		free(this->key);
	if (this->next != NULL)
		delete this->next;
}


template <class T>
HashMap<T>::HashMap(unsigned int size)
{
	InitializeCriticalSection(&MapAccess);
	items = new HashMapItem<T>[size];
	for (int i = 0; i < (int)size; i++)
	{
		items[i].next = NULL;
		items[i].key = NULL;
	}

	this->size = size;
	count = 0;
}

template <class T>
HashMap<T>::~HashMap()
{
	delete[] items;
	DeleteCriticalSection(&MapAccess);
}


template <class T>
long long HashMap<T>::GetHash(const char* key)
{
	int p = 53; // english alphabet upper and lowercase
	int m = (int)(1e9 + 9); //very large number
	long long powerOfP = 1;
	long long hashVal = 0;
	for (int i = 0; i < (int)strlen(key); i++)
	{
		hashVal = (hashVal + (key[i] - 'a' + 1) * powerOfP) % m;
		powerOfP = (powerOfP * p) % m;
	}
	return hashVal;
}

template <class T>
void HashMap<T>::Insert(const char* key, T value)
{
	unsigned long long hashValue = GetHash(key);

	EnterCriticalSection(&MapAccess);
	int index = hashValue % this->size;
	if (items[index].key == NULL)
	{
		items[index].key = _strdup(key);
		items[index].value = value;
		count++;
		LeaveCriticalSection(&MapAccess);
		return;
	}

	if (strcmp(items[index].key, key) == 0)
	{
		items[index].value = value;
		LeaveCriticalSection(&MapAccess);
		return;
	}

	HashMapItem<T>* node = items + index;
	HashMapItem<T>* nodeNext = node->next;
	while (nodeNext != NULL)
	{
		if (strcmp(nodeNext->key, key) == 0)
		{
			nodeNext->value = value;
			LeaveCriticalSection(&MapAccess);
			return;
		}
		node = node->next;
		nodeNext = nodeNext->next;
	}

	char* keyCopy = _strdup(key);
	HashMapItem<T>* newNode = new HashMapItem<T>(keyCopy, value);
	node->next = newNode;
	count++;
	free(keyCopy);

	LeaveCriticalSection(&MapAccess);
	return;
}

template <class T>
void HashMap<T>::Delete(const char* key)
{
	unsigned long long hashValue = GetHash(key);
	EnterCriticalSection(&MapAccess);
	if (count == 0)
	{
		LeaveCriticalSection(&MapAccess);
		return;
	}

	unsigned int index = hashValue % this->size;
	if (items[index].key == NULL)
	{
		LeaveCriticalSection(&MapAccess);
		return;
	}

	HashMapItem<T>* node = items + index;

	if (strcmp(node->key, key) != 0)
	{
		HashMapItem<T>* nodeNext = node->next;
		while (nodeNext != NULL)
		{
			if (strcmp(nodeNext->key, key) == 0)
			{
				node->next = nodeNext->next;
				nodeNext->next = NULL;
				free(nodeNext->key);
				nodeNext->key = NULL;
				delete nodeNext;
				count--;
				break;
			}

			node = node->next;
			nodeNext = nodeNext->next;
		}
		LeaveCriticalSection(&MapAccess);
		return;
	}

	if (node->next == NULL)
	{
		free(node->key);
		node->key = NULL;
		count--;
		LeaveCriticalSection(&MapAccess);
		return;
	}

	HashMapItem<T>* nodeNext = node->next;
	while (nodeNext->next != NULL)
	{
		node = node->next;
		nodeNext = nodeNext->next;
	}

	node->next = NULL;
	HashMapItem<T>* first = items + index;
	first->value = nodeNext->value;
	free(first->key);
	first->key = _strdup(key);

	free(nodeNext->key);
	nodeNext->key = NULL;
	delete nodeNext;
	count--;
	LeaveCriticalSection(&MapAccess);
	return;
}

template <class T>
bool HashMap<T>::Get(const char* key, T* value)
{
	unsigned  long long hashValue = GetHash(key);

	EnterCriticalSection(&MapAccess);
	if (count == 0)
	{
		LeaveCriticalSection(&MapAccess);
		return false;
	}

	unsigned int index = hashValue % this->size;
	if (items[index].key == NULL)
	{
		LeaveCriticalSection(&MapAccess);
		return false;
	}

	HashMapItem<T>* node = items + index;
	while (node != NULL)
	{
		if (strcmp(node->key, key) == 0)
		{
			*value = node->value;
			LeaveCriticalSection(&MapAccess);
			return true;
		}

		node = node->next;
	}

	LeaveCriticalSection(&MapAccess);
	return false;
}

template <class T>
bool HashMap<T>::ContainsKey(const char* key)
{
	unsigned long long hashValue = GetHash(key);
	EnterCriticalSection(&MapAccess);
	if (count == 0)
	{
		LeaveCriticalSection(&MapAccess);
		return false;
	}

	unsigned int index = hashValue % this->size;
	if (items[index].key == NULL)
	{
		LeaveCriticalSection(&MapAccess);
		return false;
	}
	
	HashMapItem<T>* node = items + index;
	while (node != NULL)
	{
		if (strcmp(node->key, key) == 0)
		{
			LeaveCriticalSection(&MapAccess);
			return true;
		}

		node = node->next;
	}

	LeaveCriticalSection(&MapAccess);
	return false;

}

template <class T>
char** HashMap<T>::GetKeys(int* keysCount)
{
	EnterCriticalSection(&MapAccess);
	*keysCount = count;
	if (*keysCount == 0)
	{
		LeaveCriticalSection(&MapAccess);
		return NULL;
	}
	char** keys = (char**)malloc(count * sizeof(char*));
	int keysFound = 0;

	for (int i = 0; i < (int)size; i++)
	{
		if (items[i].key == NULL)
		{
			continue;
		}
		keys[keysFound] = _strdup(items[i].key);
		keysFound++;
		HashMapItem<T>* node = items[i].next;
		while (node != NULL)
		{
			keys[keysFound] = _strdup(node->key);
			keysFound++;
			node = node->next;
		}
	}

	LeaveCriticalSection(&MapAccess);
	return keys;
}

template <class T>
T* HashMap<T>::GetValues(int* valuesCount)
{
	EnterCriticalSection(&MapAccess);

	int valuesFound = 0;
	char** keys = GetKeys(&valuesFound);
	if (keys == NULL)
	{
		LeaveCriticalSection(&MapAccess);
		*valuesCount = 0;
		return NULL;
	}

	T* values = (T*)malloc(valuesFound * sizeof(T));
	for (int i = 0; i < valuesFound; i++)
	{
		Get(keys[i], values + i);
		free(keys[i]);
	}

	LeaveCriticalSection(&MapAccess);
	free(keys);
	*valuesCount = valuesFound;
	return values;
}
#endif