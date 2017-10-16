#include <vector>
#include <cassert>

template <class T>
class RingBuffer {
private:
	unsigned rindex_;
	unsigned windex_;
	std::vector<T> buf_;

	unsigned mask(unsigned idx) const { return idx & (buf_.size() - 1); }
	int isPowerOfTwo(unsigned x) const { return ((x & (~x + 1)) == x); }


public:
	// capacity must be a power of two.
	explicit RingBuffer(size_t aCapacity = 0) : rindex_(0), windex_(0) {
		capacity(aCapacity);
	}
	virtual ~RingBuffer() = default;

	void capacity(size_t aCapacity) {
		assert(isPowerOfTwo(aCapacity));
		buf_.resize(aCapacity);
	}
	bool empty() const { return rindex_ == windex_; }
	bool full() const { return size() == buf_.size(); }

	size_t size() const { return windex_ - rindex_; }
	size_t available() const { return buf_.size() - size(); }

	size_t continuousSize() const {
		size_t sz = size();
		size_t cont = buf_.size() - mask(rindex_);
		return (sz < cont) ? sz : cont;
	}
	size_t continuousAvailable() const {
		size_t avil = available();
		size_t cont = buf_.size() - mask(windex_);
		return (avil < cont) ? avil : cont;
	}

	void push(T val) {
		assert(!full());
		buf_[mask(windex_++)] = val;
	}

	T pop() {
		assert(!empty());
		return buf_[mask(rindex_++)];
	}

	void discard() { rindex_ = windex_; };

	T *wdata() { return &buf_[mask(windex_)]; }
	T *rdata() { return &buf_[mask(rindex_)]; }
	void wcommit(size_t sz) { windex_ += sz; }
	void rcommit(size_t sz) { rindex_ += sz; }


	unsigned rindex() const { return rindex_; }
	T peek(unsigned idx) const {
		assert(!empty());
		return buf_[mask(idx)];
	}


	bool write(const T *buf, size_t sz) {
		if (available() < sz) return false;
		for (unsigned i=0; i<sz; i++) {
			buf_[mask(windex_++)] = buf[i];
		}
		return true;
	}

	bool read(T *buf, size_t sz) {
		if (size() < sz) return false;
		for (unsigned i=0; i<sz; i++) {
			buf[i] = buf_[mask(rindex_++)];
		}
		return true;
	}
};


template <class T>
class RingBufferDelegateBase {
public:
	RingBufferDelegateBase() {};
	virtual ~RingBufferDelegateBase() {};
	virtual void operator()(unsigned num, RingBuffer<T> bufs[]) = 0;
};


template <class OT, class RT>
class RingBufferDelegate : public RingBufferDelegateBase<RT> {
protected:
	typedef void (OT::*DelegateFunc)(unsigned num, RingBuffer<RT> bufs[]);

	OT *obj_;			// object
	DelegateFunc func_;	// function

public:
	RingBufferDelegate() {};
	virtual ~RingBufferDelegate() {};

	virtual void operator()(unsigned num, RingBuffer<RT> bufs[]) {
		(obj_->*func_)(num, bufs);
	}

	void set(OT *obj, DelegateFunc func) {
		obj_ = obj;
		func_ = func;
	}

	static RingBufferDelegateBase<RT> *create(OT* obj, DelegateFunc func) {
		RingBufferDelegate *dg = new RingBufferDelegate;
		dg->set(obj, func);
		return dg;
	}
};
