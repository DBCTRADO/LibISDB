/*
  LibISDB
  Copyright(c) 2017-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 @file   EVRHelpers.hpp
 @brief  EVR ヘルパ
 @author DBCTRADO
*/


#ifndef LIBISDB_EVR_HELPERS_H
#define LIBISDB_EVR_HELPERS_H


#include "../../../../../Utilities/Lock.hpp"


namespace LibISDB::DirectShow
{

	constexpr LONG MFTimeToMsec(LONGLONG time)
	{
		return static_cast<LONG>(time / (10000000 / 1000));
	}


	class RefCountedObject
	{
	public:
		ULONG AddRef()
		{
			return ::InterlockedIncrement(&m_RefCount);
		}

		ULONG Release()
		{
			ULONG Count = ::InterlockedDecrement(&m_RefCount);
			if (Count == 0) {
				delete this;
			}
			return Count;
		}

	protected:
		virtual ~RefCountedObject() = default;

	private:
		LONG m_RefCount = 1;
	};


	template <class T> struct NoOp
	{
		void operator () (T &t) const
		{
		}
	};

	template <class T> class LinkedList
	{
	protected:
		struct Node {
			Node *pPrev = nullptr;
			Node *pNext = nullptr;
			T Item;

			Node() = default;

			Node(T item) : Item(item)
			{
			}
		};

	public:
		class Position
		{
		public:
			friend class LinkedList<T>;

			Position() = default;

			bool operator == (const Position &rhs) const noexcept
			{
				return pNode == rhs.pNode;
			}

			bool operator != (const Position &rhs) const noexcept
			{
				return pNode != rhs.pNode;
			}

		private:
			const Node *pNode = nullptr;

			Position(Node *p)
				: pNode(p)
			{
			}
		};

		LinkedList()
		{
			m_Anchor.pNext = &m_Anchor;
			m_Anchor.pPrev = &m_Anchor;
		}

		virtual ~LinkedList()
		{
			Clear();
		}

		HRESULT InsertBack(T Item)
		{
			return InsertAfter(Item, m_Anchor.pPrev);
		}

		HRESULT InsertFront(T Item)
		{
			return InsertAfter(Item, &m_Anchor);
		}

		HRESULT RemoveBack(T *pItem = nullptr)
		{
			if (IsEmpty()) {
				return E_UNEXPECTED;
			}
			return RemoveItem(Back(), pItem);
		}

		HRESULT RemoveFront(T *pItem = nullptr)
		{
			if (IsEmpty()) {
				return E_UNEXPECTED;
			}
			return RemoveItem(Front(), pItem);
		}

		HRESULT GetBack(T *pItem)
		{
			if (IsEmpty()) {
				return E_UNEXPECTED;
			}
			return GetItem(Back(), pItem);
		}

		HRESULT GetFront(T *pItem)
		{
			if (IsEmpty()) {
				return E_UNEXPECTED;
			}
			return GetItem(Front(), pItem);
		}

		size_t GetCount() const { return m_Count; }

		bool IsEmpty() const
		{
			return m_Count == 0;
		}

		template <class TFree> void Clear(const TFree &Free)
		{
			Node *pNext = m_Anchor.pNext;

			while (pNext != &m_Anchor) {
				Free(pNext->Item);

				Node *pTmp = pNext->pNext;
				delete pNext;
				pNext = pTmp;
			}

			m_Anchor.pNext = &m_Anchor;
			m_Anchor.pPrev = &m_Anchor;

			m_Count = 0;
		}

		virtual void Clear()
		{
			Clear< NoOp<T> >(NoOp<T>());
		}

		Position FrontPosition()
		{
			if (IsEmpty()) {
				return Position(nullptr);
			}
			return Position(Front());
		}

		Position EndPosition() const
		{
			return Position();
		}

		HRESULT GetItemPos(Position Pos, T *pItem)
		{
			if (Pos.pNode == nullptr) {
				return E_INVALIDARG;
			}
			return GetItem(Pos.pNode, pItem);
		}

		Position Next(Position Pos)
		{
			if ((Pos.pNode != nullptr) && (Pos.pNode->pNext != &m_Anchor)) {
				return Position(Pos.pNode->pNext);
			} else {
				return Position(nullptr);
			}
		}

		HRESULT Remove(Position &Pos, T *pItem)
		{
			if (Pos.pNode == nullptr) {
				return E_INVALIDARG;
			}

			Node *pNode = const_cast<Node*>(Pos.pNode);

			Pos = Position();

			return RemoveItem(pNode, pItem);
		}

	protected:
		Node m_Anchor;
		size_t m_Count = 0;

		Node *Front() const
		{
			return m_Anchor.pNext;
		}

		Node *Back() const
		{
			return m_Anchor.pPrev;
		}

		virtual HRESULT InsertAfter(T item, Node *pBefore)
		{
			if (pBefore == nullptr) {
				return E_POINTER;
			}

			Node *pNode = new(std::nothrow) Node(item);
			if (pNode == nullptr) {
				return E_OUTOFMEMORY;
			}

			Node *pAfter = pBefore->pNext;

			pBefore->pNext = pNode;
			pAfter->pPrev = pNode;

			pNode->pPrev = pBefore;
			pNode->pNext = pAfter;

			m_Count++;

			return S_OK;
		}

		virtual HRESULT GetItem(const Node *pNode, T *pItem)
		{
			if ((pNode == nullptr) || (pItem == nullptr)) {
				return E_POINTER;
			}

			*pItem = pNode->Item;

			return S_OK;
		}

		virtual HRESULT RemoveItem(Node *pNode, T *pItem)
		{
			if (pNode == nullptr) {
				return E_POINTER;
			}
			if (pNode == &m_Anchor) {
				return E_INVALIDARG;
			}

			pNode->pNext->pPrev = pNode->pPrev;
			pNode->pPrev->pNext = pNode->pNext;

			if (pItem != nullptr) {
				*pItem = pNode->Item;
			}

			delete pNode;
			m_Count--;

			return S_OK;
		}
	};


	struct ComAutoRelease
	{
		void operator () (IUnknown *p) const
		{
			if (p != nullptr) {
				p->Release();
			}
		}
	};

	template<class T> struct AutoDelete
	{
		void operator () (T *p) const
		{
			if (p != nullptr) {
				delete p;
			}
		}
	};


	template <class T, bool NULLABLE = false> class ComPtrList : public LinkedList<T *>
	{
	public:
		void Clear() override
		{
			LinkedList<T*>::Clear(ComAutoRelease());
		}

		~ComPtrList()
		{
			Clear();
		}

	protected:
		typedef typename LinkedList<T *>::Node Node;

		HRESULT InsertAfter(T *pItem, Node *pBefore) override
		{
			if (!pItem && !NULLABLE) {
				return E_POINTER;
			}

			HRESULT hr = LinkedList<T *>::InsertAfter(pItem, pBefore);

			if (SUCCEEDED(hr)) {
				if (pItem) {
					pItem->AddRef();
				}
			}

			return hr;
		}

		HRESULT GetItem(const Node *pNode, T **ppItem) override
		{
			T *pItem = nullptr;
			HRESULT hr = LinkedList<T *>::GetItem(pNode, &pItem);

			if (SUCCEEDED(hr)) {
				*ppItem = pItem;
				if (pItem) {
					pItem->AddRef();
				}
			}

			return hr;
		}

		HRESULT RemoveItem(Node *pNode, T **ppItem) override
		{
			T *pItem = nullptr;
			HRESULT hr = LinkedList<T *>::RemoveItem(pNode, &pItem);

			if (SUCCEEDED(hr)) {
				if (ppItem) {
					*ppItem = pItem;
				} else {
					if (pItem) {
						pItem->Release();
					}
				}
			}

			return hr;
		}
	};

	typedef ComPtrList<IMFSample> VideoSampleList;


	template <class T> class ThreadSafeQueue
	{
	public:
		HRESULT Queue(T *p)
		{
			BlockLock Lock(m_Lock);
			return m_List.InsertBack(p);
		}

		HRESULT Dequeue(T **pp)
		{
			BlockLock Lock(m_Lock);

			if (m_List.IsEmpty()) {
				*pp = nullptr;
				return S_FALSE;
			}

			return m_List.RemoveFront(pp);
		}

		HRESULT PutBack(T *p)
		{
			BlockLock Lock(m_Lock);
			return m_List.InsertFront(p);
		}

		void Clear() 
		{
			BlockLock Lock(m_Lock);
			m_List.Clear();
		}

	private:
		MutexLock m_Lock; 
		ComPtrList<T> m_List;
	};


	template<class T> class AsyncCallback : public IMFAsyncCallback
	{
	public: 
		typedef HRESULT (T::*InvokeFn)(IMFAsyncResult *pAsyncResult);

		AsyncCallback(T *pParent, InvokeFn fn)
			: m_pParent(pParent)
			, m_pInvokeFn(fn)
		{
		}

		STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
		{
			if (ppv == nullptr) {
				return E_POINTER;
			}

			if (riid == __uuidof(IUnknown)) {
				*ppv = static_cast<IUnknown *>(static_cast<IMFAsyncCallback *>(this));
			} else if (riid == __uuidof(IMFAsyncCallback)) {
				*ppv = static_cast<IMFAsyncCallback *>(this);
			} else {
				*ppv = nullptr;
				return E_NOINTERFACE;
			}

			AddRef();

			return S_OK;
		}

		STDMETHODIMP_(ULONG) AddRef() override
		{
			//return m_pParent->AddRef();
			return 1;
		}

		STDMETHODIMP_(ULONG) Release() override
		{
			//return m_pParent->Release();
			return 1;
		}

		STDMETHODIMP GetParameters(DWORD *pdwFlags, DWORD *pdwQueue) override
		{
			return E_NOTIMPL;
		}

		STDMETHODIMP Invoke(IMFAsyncResult *pAsyncResult) override
		{
			return (m_pParent->*m_pInvokeFn)(pAsyncResult);
		}

		T *m_pParent;
		InvokeFn m_pInvokeFn;
	};


	class SamplePool
	{
	public:
		SamplePool();
		virtual ~SamplePool();

		HRESULT Initialize(VideoSampleList &Samples);
		HRESULT Clear();

		HRESULT GetSample(IMFSample **ppSample);
		HRESULT ReturnSample(IMFSample *pSample);
		bool AreSamplesPending();

	private:
		MutexLock m_Lock;

		VideoSampleList m_VideoSampleQueue;
		bool m_Initialized;
		size_t m_PendingCount;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_EVR_HELPERS_H
