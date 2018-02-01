#ifndef JResettable_h
#define JResettable_h

//primarily for JResourcePool objects
class JResettable
{
	public:
		virtual ~JResettable(void) = 0;
		virtual void Release(void){}; //Release all (pointers to) resources, called when recycled to pool
		virtual void Reset(void){}; //Re-initialize the object, called when retrieved from pool
};

inline JResettable::~JResettable(void) { }

#endif // JResettable_h
