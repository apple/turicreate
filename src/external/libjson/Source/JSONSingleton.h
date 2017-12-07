#ifndef JSONSINGLETON_H
#define JSONSINGLETON_H

template <typename T> class JSONSingleton {
public:
	static inline T get(void){ 
		return get_singleton() -> ptr; 
	}
	static inline void set(T p){ 
		get_singleton() -> ptr = p; 
	}
private:
	inline JSONSingleton() : ptr(NULL) { }
	JSONSingleton(const JSONSingleton<T> &);
	JSONSingleton<T> operator = (const JSONSingleton<T> &);
	static inline JSONSingleton<T> * get_singleton(void){
		static JSONSingleton<T> instance;
		return &instance;
	}
	T ptr;
};

#endif
