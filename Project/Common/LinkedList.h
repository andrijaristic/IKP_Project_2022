#pragma once

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <iostream>
#include <cstdlib>
#include <Windows.h>
using namespace std;

//class for linked list node
template <class T>
class ListItem
{
private:
	T value;
	ListItem* next;
	ListItem* previous;

public:
	ListItem();

	template <class T> friend class LinkedList;
};

// Class for Linked list
template <class T>
class LinkedList
{
private:
	CRITICAL_SECTION ListAccess;			 // Critical secion for thread safety
	ListItem<T>* head;			// List head
	ListItem<T>* tail;			// List tail
	int count;					// current number of elements in the list

public:
	LinkedList();											// Constructor
	~LinkedList();											// Destructor
	bool PushFront(T element);								// Adds element to the front of the list, returns true if succeeds
	bool PushBack(T element);								// Adds element to the back of the list, returns true if succeeds
	bool PopFront(T* value);								// Gets element from the front of the list, returns true if succeeds or false if list is empty
	bool PopBack(T* value);									// Gets element from the back of the list, returns true if succeeds or false if list is empty
	int Count();											// Get list elements count
	bool isEmpty();											// Check if empty
};

template <class T>
ListItem<T>::ListItem()
{
	this->next = NULL;
	this->previous = NULL;
}

template <class T>
LinkedList<T>::LinkedList()
{
	head = NULL;
	count = 0;
	tail = head;
	InitializeCriticalSection(&ListAccess);

}

template <class T>
LinkedList<T>::~LinkedList()
{
	ListItem<T>* temp = head;
	if (temp != NULL)
	{
		ListItem<T>* tempNext = head->next;
		while (tempNext != NULL)
		{
			delete temp;
			temp = tempNext;
			tempNext = tempNext->next;
		}
		delete temp;
	}
	DeleteCriticalSection(&ListAccess);
}



template <class T>
bool LinkedList<T>::isEmpty()
{
	return count == 0;
}

template <class T>
int LinkedList<T>::Count()
{
	return count;
}

template <class T>
bool LinkedList<T>::PushFront(T element)
{
	EnterCriticalSection(&ListAccess);

	if (count == 0)
	{
		ListItem<T>* newItem = new ListItem<T>();
		newItem->value = element;
		newItem->previous = NULL;
		newItem->next = NULL;
		head = newItem;
		tail = newItem;
		count++;
		LeaveCriticalSection(&ListAccess);
		return true;
	}

	ListItem<T>* newItem = new ListItem<T>();
	newItem->value = element;
	newItem->previous = NULL;
	newItem->next = head;
	head->previous = newItem;
	head = newItem;
	count++;
	LeaveCriticalSection(&ListAccess);
	return true;
}

template <class T>
bool LinkedList<T>::PushBack(T element)
{
	EnterCriticalSection(&ListAccess);

	if (count == 0)
	{
		ListItem<T>* newItem = new ListItem<T>();
		newItem->value = element;
		newItem->previous = NULL;
		newItem->next = NULL;
		head = newItem;
		tail = newItem;
		count++;
		LeaveCriticalSection(&ListAccess);
		return true;
	}

	ListItem<T>* newItem = new ListItem<T>();
	newItem->value = element;
	newItem->previous = tail;
	newItem->next = NULL;
	tail->next = newItem;
	tail = newItem;
	count++;
	LeaveCriticalSection(&ListAccess);
	return true;
}

template <class T>
bool LinkedList<T>::PopFront(T* value)
{
	EnterCriticalSection(&ListAccess);

	if (count == 0)
	{
		LeaveCriticalSection(&ListAccess);
		return false;
	}

	ListItem<T>* item = head;
	*value = item->value;

	if (count == 1)
	{
		head = NULL;
		tail = NULL;
	}
	else
	{
		head = item->next;
		head->previous = NULL;
	}

	item->next = NULL;
	item->previous = NULL;
	delete item;
	count--;
	LeaveCriticalSection(&ListAccess);
	return true;
}

template <class T>
bool LinkedList<T>::PopBack(T* value)
{
	EnterCriticalSection(&ListAccess);

	if (count == 0)
	{
		LeaveCriticalSection(&ListAccess);
		return false;
	}

	ListItem<T>* item = tail;
	*value = item->value;

	if (count == 1)
	{
		head = NULL;
		tail = NULL;
	}
	else
	{
		tail = item->previous;
		tail->next = NULL;
	}

	item->next = NULL;
	item->previous = NULL;
	delete item;
	count--;
	LeaveCriticalSection(&ListAccess);
	return true;
}
#endif

