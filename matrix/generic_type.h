#ifndef __GENERAL_TYPE_H__
#define __GENERAL_TYPE_H__

/*
 * Copyright 2014 Open Connectome Project (http://openconnecto.me)
 * Written by Da Zheng (zhengda1936@gmail.com)
 *
 * This file is part of FlashMatrix.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

namespace fm
{

/**
 * Here defines the primitive types.
 */
enum prim_type
{
	P_CHAR,
	P_SHORT,
	P_INTEGER,
	P_LONG,
	P_FLOAT,
	P_DOUBLE,
	P_BOOL,
	NUM_TYPES,
};

template<class T>
prim_type get_type()
{
	return prim_type::NUM_TYPES;
}

template<>
inline prim_type get_type<char>()
{
	return prim_type::P_CHAR;
}

template<>
inline prim_type get_type<short>()
{
	return prim_type::P_SHORT;
}

template<>
inline prim_type get_type<int>()
{
	return prim_type::P_INTEGER;
}

template<>
inline prim_type get_type<long>()
{
	return prim_type::P_LONG;
}

template<>
inline prim_type get_type<float>()
{
	return prim_type::P_FLOAT;
}

template<>
inline prim_type get_type<double>()
{
	return prim_type::P_DOUBLE;
}

template<>
inline prim_type get_type<bool>()
{
	return prim_type::P_BOOL;
}

/**
 * This class defines a generic type for a scalar variable.
 * It shouldn't be used in an array because it has a lot of overhead.
 */
class scalar_type
{
public:
	/**
	 * Get the raw representation of the type.
	 */
	virtual const char *get_raw() const = 0;
	/**
	 * The size of the type.
	 */
	virtual size_t get_size() const = 0;
	/**
	 * Set the value of the scalar variable in the raw representation.
	 */
	virtual bool set_raw(const char *v, int size) = 0;
};

template<class T>
class scalar_type_impl: public scalar_type
{
	T v;
public:
	virtual const char *get_raw() const {
		return (const char *) &v;
	}

	virtual size_t get_size() const {
		return sizeof(T);
	}

	virtual bool set_raw(const char *v, int size) {
		if (sizeof(T) != size)
			return false;

		memcpy(&this->v, v, size);
		return true;
	}

	T get() const {
		return v;
	}

	void set(T v) {
		this->v = v;
	}
};

}

#endif
