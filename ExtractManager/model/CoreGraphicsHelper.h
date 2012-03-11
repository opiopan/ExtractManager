//
//  CoreGraphicsHelper.h
//

#ifndef CoreGraphicsHelper_h
#define CoreGraphicsHelper_h

template <class T, T (*RETAIN)(T), void (*RELEASE)(T)> class ECGObjectRef{
protected:
    T ref;
    
public:
    ECGObjectRef():ref(NULL){};
    explicit ECGObjectRef(T r):ref(r){};
    explicit ECGObjectRef(ECGObjectRef& r):ref(r.ref){
        RETAIN(ref);
    };
    ~ECGObjectRef(){
        if (ref){
            RELEASE(ref);
        }
    };
    
    operator T () const {return ref;};

    ECGObjectRef& operator = (ECGObjectRef& src){
        if (ref){
            RELEASE(ref);
        };
        ref = src.ref;
        if (ref){
            RETAIN(ref);
        }
        return *this;
    };
    ECGObjectRef& operator = (T src){
        if (ref){
            RELEASE(ref);
        }
        ref = src;
        return *this;
    };
};

#define DEF_ECGREF(T) typedef ECGObjectRef<T##Ref, T##Retain, T##Release> E##T##Ref

DEF_ECGREF(CGContext);
DEF_ECGREF(CGColor);
DEF_ECGREF(CGColorSpace);
DEF_ECGREF(CGImage);
DEF_ECGREF(CGPath);

#endif
