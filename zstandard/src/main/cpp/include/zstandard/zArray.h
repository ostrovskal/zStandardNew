
#pragma once

template <typename T, bool> struct release_node { static void release(const T& t) { /*static_assert(false, "release_node invalid!");*/ } };
template <typename T> struct release_node < T, false > { static void release(const T& t) { t.~T(); } static T dummy() { return T(); } };
template <typename T> struct release_node < T, true > { static void release(const T& t) { delete t; } static T dummy() { return nullptr; } };

template <typename T, typename R, bool> struct get_node { static bool compare(const T& t, const R& r) { } };
template <typename T, typename R> struct get_node < T, R, false > { static bool compare(const T& t, const R& r) { return t == r; } };
template <typename T, typename R> struct get_node < T, R, true > { static bool compare(const T& t, const R& r) { return *t == r; } };

#define Z_RELEASE_NODE(T, V)	release_node<T, (bool)std::is_pointer<T>()>::release(V)
#define Z_GET_NODE(T, R, v, r)	get_node<T, R, (bool)std::is_pointer<T>()>::compare(v, r)

template <typename T> class zArray {
public:
	// конструктор по умолчанию
	zArray() { init(); }
	// инициализирующий конструктор
	zArray(int _count) { init(); resize(_count); }
	// конструктор копии
	zArray(const zArray<T>& src) { *this = src; }
	// конструктор списка инициализации
	zArray(const std::initializer_list<T>& src) noexcept { for(auto t : src) *this += t; }
	// конструктор переноса
	zArray(zArray<T>&& src) noexcept {
		data = src.data; 
		count = src.count; 
		max_count = src.max_count; 
		src.init(); 
	}
	// деструктор
	~zArray() { clear(); }
	// освободить память, без сброса
	void free() { delete (u8*)data; init(); }
	// сброс
	void clear() noexcept { erase(0, count, true); free(); }
	// установить размер
	void resize(int _count) noexcept { clear(); alloc(_count); count = _count; }
	// вернуть признак заполненного массива
	bool isNotEmpty() const { return count > 0; }
	// вернуть признак пустого массива
	bool isEmpty() const { return count == 0; }
	// добавить элемент
	T& operator += (const T& elem) noexcept { return insert(count, elem); }
	// добавить массив
	const zArray& operator += (const zArray<T>& src) noexcept { return insert(count, src.data(), src.size()); }
	// заместить массив
	const zArray& operator = (const zArray<T>& src) noexcept { clear(); return insert(0, src.get_data(), src.size()); }
	// оператор индекса
	T& operator [] (int idx) const noexcept { return at(idx); }
	// вернуть последний элемент
	T& last() const noexcept { return data[count - 1]; }
    // обмен значениями
    void swap(int idx1, int idx2) {
        if(idx1 < count && idx2 < count) {
			std::swap(data[idx1], data[idx2]);
        }
    }
	// оператор переноса
	const zArray& operator = (zArray<T>&& src) noexcept {
		clear();
		data = src.data; count = src.count; max_count = src.max_count;
		src.init();
		return *this;
	}
	// установка элемента по индексу
	T& set(int idx, const T& elem) noexcept {
		if(idx < count) {
			Z_RELEASE_NODE(T, data[idx]);
			data[idx] = std::move(elem);
		}
		return data[idx];
	}
	// вставка элемента по индексу
	T& insert(int idx, const T& elem) noexcept {
		if(idx >= 0 && idx <= count) {
			alloc(1);
            auto _sizeof(sizeof(T));
			auto size((max_count - (idx + 1)) * _sizeof);
			memmove(data + idx + 1, data + idx, size);
			data[idx] = std::move(elem); count++;
		}
		return data[idx];
	}
	// удаление элементов
	const zArray& erase(int idx, int _count, bool _delete = true) noexcept {
		if(_count && idx >= 0 && idx < count) {
			if((idx + _count) > count) _count = (count - idx);
			if(_delete) for(int i = 0; i < _count; i++) { Z_RELEASE_NODE(T, data[idx + i]); }
            auto _sizeof(sizeof(T));
			auto size((max_count - idx - _count) * _sizeof);
			memmove(data + idx, data + idx + _count, size);
			count -= _count;
		}
		return *this;
	}
	// найти элемент
	template<typename R> int indexOf(const R& r) const noexcept {
		for(auto i = 0; i < count; i++) {
			if(Z_GET_NODE(T, R, data[i], r)) return i;
		}
		return -1;
	}
	// диапазонный цикл
	T* begin() const noexcept { return data; }
	T* end() const noexcept { return &data[count]; }
	// вернуть размер
	int size() const noexcept { return count; }
	// оператор индекса
	T& at(int idx) const noexcept { return data[idx]; }
	// вернуть указатель на данные
	const T* get_data() const noexcept { return (const T*)data; }
	T* get_data() noexcept { return data; }
protected:
	// вставка нескольких элементов
	const zArray& insert(int idx, const T* elem, int _count) {
		alloc(_count);
		while(_count--) { insert(idx, elem[idx]); idx++; }
		return *this;
	}
	// очистить
	void init() { count = max_count = 0; data = nullptr; }
	// выделение памяти
	void alloc(int _count) {
		if((count + _count) >= max_count) {
			max_count = (count + _count + 8);
			// выделяем блок
			T* ptr((T*)new u8[max_count * sizeof(T)]);
			memset(ptr, 0, max_count * sizeof(T));
			// копируем старые данные, если есть
			if(data) {
				memcpy(ptr, data, count * sizeof(T));
				delete (u8*)data;
			}
			data = ptr;
		}
	}
	// количество элементов
	int count{0};
	// выделено элементов
	int max_count{0};
	// данные
	T* data{nullptr};
};
