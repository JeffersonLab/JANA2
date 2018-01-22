#ifndef JResettable_h
#define JResettable_h

//primarily for JResourcePool objects
class JResettable
{
	public:
		virtual ~JResettable(void){}
		virtual void Release(void) = 0; //Release all (pointers to) resources, called when recycled to pool
		virtual void Reset(void) = 0; //Re-initialize the object, called when retrieved from pool
};

#endif // JResettable_h
