#include "base.h"
#include <array>
#include <limits>
#include <vector>

template <typename T>
class ObjArena 
{
    u64 _capacity = 0;
    std::vector<T> _data;
    std::vector<u64> _counters;
    u64 _firstAvailableIdx;

    void _allocateMore(u64 atleast = 0) 
    {
        auto more = _capacity + atleast;
        auto newCap = _capacity + more;
        _data.resize(newCap);
        _counters.resize(newCap, 1);
        //std::cout << newCap << ": ";
        for (u64 i = newCap - 1; i >= 0 && i < newCap; --i) {
            if (_counters[i] == 0)
                break;
            else
                _counters[i] += (i < _capacity) ? more : (more - i - 1);
        }
        //for (u64 i = 0; i < newCap; ++i)
        //    std::cout << _counters[i] << " ";

        if (_firstAvailableIdx >= _capacity)
            _firstAvailableIdx = _capacity;
        _capacity = newCap;
    }

public:
    
    ObjArena(u64 capacity) :
        _firstAvailableIdx(0)
    {
        _allocateMore(capacity);
    }

    u64 acquire(const T& obj, u64 count = 1) {
        auto _candidx = _firstAvailableIdx; 
        while (_counters[_candidx] < count) {
            _candidx++;
            if (_candidx == _capacity)
                _allocateMore();
        }
        bool enough = _counters[_candidx] >= count;
        if (enough) {
            auto idx = _candidx;
            _data[idx] = obj;
            for (u64 i = 0; i < count; ++i)
                _counters[idx + i] = 0;
            if (idx == _firstAvailableIdx)
                _firstAvailableIdx += count;
            while (_firstAvailableIdx < _capacity && !_counters[_firstAvailableIdx])
                _firstAvailableIdx++;
            return idx;
        }
        return -1;        
    }

    T& at(u64 idx) {
        return _data[idx];
    }

    u64 count() {return _firstAvailableIdx;}
    u64 capacity() {return _capacity;}

};

template <typename T>
class SharedObjArena;

template <typename T>
class SharedObjPtr 
{
    friend class SharedObjArena<T>;

    SharedObjArena<T>* _arena = nullptr;
    u64 _idx = std::numeric_limits<u64>::max();

    u64* _get_counter();
    void _inc_counter();
    void _dec_counter();
    
    bool _empty = true;

public:

    SharedObjPtr() = default;
    SharedObjPtr(SharedObjArena<T>* _arena, u64 idx);

    SharedObjPtr(SharedObjPtr<T>& sp) {
        _arena = sp._arena;
        _idx = sp._idx;
        _inc_counter();
    }

    ~SharedObjPtr();

    T* get();

    u64 ref_count() {
        return *_get_counter();
    }

    T& operator*() { 
      return *get(); 
    }

    T* operator->() { 
      return get();
    }

    bool empty() { return _empty; }

    u64 idx() { return _idx; }
    
    void operator= (const SharedObjPtr& sp);

    operator bool() const { return !_empty; }
};

template <typename T> 
class SharedObjArena
{
    friend class SharedObjPtr<T>;

    u64 _capacity;
    std::vector<T> _data;
    std::vector<u64> _counters;
    u64 _firstAvailableIdx;

    void _allocateMore() {
        auto newCap = _capacity * 2;
        _data.resize(newCap);
        _counters.resize(newCap);
        _firstAvailableIdx = _capacity;
        _capacity = newCap;
    }

    void _release(SharedObjPtr<T>& ptr) {
        _counters[ptr._idx] = 0;
        if (ptr._idx < _firstAvailableIdx)
            _firstAvailableIdx = ptr._idx;
        ptr._empty = true;
    }

public:

    SharedObjArena(u64 capacity) :
        _capacity(capacity),
        _data(std::vector<T>(capacity)),
        _counters(std::vector<u64>(capacity)),
        _firstAvailableIdx(0)
    {}

    SharedObjPtr<T> acquire(const T& obj) {        
        auto ptr = _data.data() + _firstAvailableIdx;
        *ptr = obj;
        auto idx = _firstAvailableIdx;
        _counters[_firstAvailableIdx] = 1;
        _firstAvailableIdx++;
        while (_firstAvailableIdx < _capacity && _counters[_firstAvailableIdx])
            _firstAvailableIdx++;
        if (_firstAvailableIdx >= _capacity)
            _allocateMore();
        return SharedObjPtr<T>(this, idx);
    }

    u64 capacity() {return _capacity;}
};

template <typename T>
SharedObjPtr<T>::SharedObjPtr(SharedObjArena<T>* _arena, u64 idx) :
    _arena(_arena),
    _idx(idx),
    _empty(false)
{
    std::cout << "ptr created!\n";
}

template <typename T>
SharedObjPtr<T>::~SharedObjPtr() {
    if (!_empty) {
        _dec_counter();
        std::cout << "ptr destroyed!\n";
    }
}

template <typename T>
T* SharedObjPtr<T>::get() { 
    return _arena->_data.data() + _idx;
}

template <typename T>
u64* SharedObjPtr<T>::_get_counter() { 
    return (_arena->_counters.data() + _idx);
}

template <typename T>
void SharedObjPtr<T>::_inc_counter() { 
    (*_get_counter())++;
}

template <typename T>
void SharedObjPtr<T>::_dec_counter() { 
    (*_get_counter())--;
    if(!ref_count())
        _arena->_release(*this);
}

template <typename T>
void SharedObjPtr<T>::operator= (const SharedObjPtr& sp) {
    if (!_empty && (_idx != sp._idx || _arena != sp._arena)) {
        _dec_counter();
    } else {
        _idx = sp._idx;
        _arena = sp._arena;
        _empty = sp._empty;
        if (!_empty)
            _inc_counter();
    }
}

template <typename T, u64 N>
class SharedObjPtrArray {
    SharedObjArena<T>& _arena;
    std::array<SharedObjPtr<T>, N> _array;
    u64 _size = 0;

public:
    SharedObjPtrArray(SharedObjArena<T>& arena) :
        _arena(arena)
    { }

    SharedObjPtr<T> acquire(const T &obj) {
        if (_size < N) {
            auto _new = _arena.acquire(obj);
            _array[_size] = _new;
            while (_size < N && !_array[_size].empty())
                _size++;
            return _new;
        }
        return SharedObjPtr<T>();
    }

    SharedObjPtr<T> at(u64 idx) {
        return _array[idx];
    }

    u64 size() {
        return _size;
    }

    void release(u64 idx) {
        _arena._release(_array[idx]);
    }
};