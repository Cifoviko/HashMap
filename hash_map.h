#include <iostream>
#include <functional>
#include <exception>
#include <list>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    struct Cell;

    struct Ptr {
    public:
        explicit Ptr(Cell* cell = nullptr) : cell(cell) {}

        Cell* cell = nullptr; // Cell assigned to this ptr
        Ptr* next = nullptr; // Next ptr in list
        Ptr* previous = nullptr; // Previous ptr in list
    };

    struct Cell { // Cell of hash table
    public:
        enum State { // Possible states of cell
            EMPTY,
            FULL,
            DELETED
        };

        std::pair<KeyType, ValueType> element; // element in cell

        size_t hash = 0;            // Hash values of key in cell
        State  validation = State::EMPTY; // Cell status
        Ptr* ptr = nullptr;      // Ptr assigned for this cell
    };

    explicit HashMap(const Hash& hasher = Hash()) : _hasher(hasher) {
        // Creates empty map with STARTING_CAPACITY

        resize_(_STARTING_CAPACITY);
    }

    explicit HashMap(const HashMap& map) {
        // Creates identical map by elements

        // Copying capacity
        resize_(map._capacity);

        // Copying elements
        auto it = map.begin();
        while (it != map.end()) {
            hard_insert_(*it);
            ++it;
        }
    }

    template<typename Iterator>
    HashMap(Iterator begin, Iterator end, const Hash& hasher = Hash()) : _hasher(hasher) {
        // Creates map with list of elements

        resize_(_STARTING_CAPACITY);
        while (begin != end) {
            insert(*begin);
            ++begin;
        }
    }

    HashMap(const std::initializer_list<std::pair<KeyType, ValueType>>& initializer_list, const Hash& hasher = Hash()) : _hasher(hasher) {
        // Creates map with list of elements & _CAPACITY_MULTIPLIER of list size capacity

        resize_(initializer_list.size() * _CAPACITY_MULTIPLIER);
        for (const auto& elemet : initializer_list) {
            insert(elemet);
        }
    }

    ~HashMap() {
        // Clears all using memory
        _deletePtrs();
        delete[] _data;
    }

    void insert(const std::pair<const KeyType, const ValueType>& element) {
        // Insert element into the map if there is none with that key

        // Calculates hash
        size_t hash = _hasher(element.first);

        // Tries to find target element
        std::pair<bool, size_t> position = find_element_with_hash_(element.first, hash);

        // Checks if there is such element
        if (!position.first) {
            hard_insert_with_hash_(element, hash);
        }
    }

    void erase(const KeyType& key) {
        // Erases element from map

        // Tries to find target element
        std::pair<bool, size_t> position = find_element_(key);

        // Checks if there is target element & deletes it
        if (position.first) {
            delete_(position.second);
        }
    }

    class iterator {
    public:
        explicit iterator(Ptr* it = nullptr) : _it(it) {}

        iterator& operator++() {
            _find_next();
            return *this;
        }

        iterator operator++(int) {
            iterator copy = *this;
            _find_next();
            return copy;
        }

        iterator& operator--() {
            _find_next();
            return *this;
        }

        iterator operator--(int) {
            iterator copy = *this;
            _find_next();
            return copy;
        }

        bool operator==(const iterator& ptr) const {
            return (_it == ptr._it);
        }

        bool operator!=(const iterator& ptr) const {
            return (_it != ptr._it);
        }

        std::pair<const KeyType, ValueType>& operator*() const {
            return *operator->();
        }

        std::pair<const KeyType, ValueType>* operator->() const {
            return reinterpret_cast<std::pair<const KeyType, ValueType>*>(&(_it->cell->element));
        }

    private:
        HashMap::Ptr* _it;   // Target ptr in hash table

        void _find_next() {
            // Finds next filled cell

            if (_it != nullptr) {
                _it = _it->next;
            }
        }

        void _find_previous() {
            // Finds previous filled cell

            if (_it != nullptr) {
                _it = _it->previous;
            }
        }
    };

    iterator begin() {
        return iterator(_ptr);
    }

    iterator end() {
        return iterator(nullptr);
    }

    class const_iterator {
    public:
        explicit const_iterator(Ptr* it = nullptr) : _it(it) {}

        const_iterator& operator++() {
            _find_next();
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator copy = *this;
            _find_next();
            return copy;
        }

        const_iterator& operator--() {
            _find_next();
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator copy = *this;
            _find_next();
            return copy;
        }

        bool operator==(const const_iterator& ptr) const {
            return (_it == ptr._it);
        }

        bool operator!=(const const_iterator& ptr) const {
            return (_it != ptr._it);
        }

        const std::pair<const KeyType, ValueType>& operator*() const {
            return *operator->();
        }

        const std::pair<const KeyType, ValueType>* operator->() const {
            return reinterpret_cast<std::pair<const KeyType, ValueType>*>(&(_it->cell->element));
        }

    private:
        HashMap::Ptr* _it;   // Target cell in hash table

        void _find_next() {
            // Finds next filled cell

            if (_it != nullptr) {
                _it = _it->next;
            }
        }

        void _find_previous() {
            // Finds previous filled cell

            if (_it != nullptr) {
                _it = _it->previous;
            }
        }
    };

    const_iterator begin() const {
        return const_iterator(_ptr);
    }

    const_iterator end() const {
        return const_iterator(nullptr);
    }

    HashMap& operator=(const HashMap& map) {
        // Makes identical map by elements

        // Copying capacity
        resize_(map._capacity);

        // Copying elements
        auto it = map.begin();
        while (it != map.end()) {
            hard_insert_(*it);
            ++it;
        }

        return (*this);
    }

    ValueType& operator[](KeyType key) {
        // Acess value at target key, if there is none, creates new

        // Calculates hash
        size_t hash = _hasher(key);

        // Finds target element
        std::pair<bool, size_t> position = find_element_with_hash_(key, hash);
        if (position.first) {
            return _data[position.second].element.second;
        }
        else {
            hard_insert_with_hash_(std::make_pair(key, ValueType()), hash);
            position = find_element_with_hash_(key, hash);
            return _data[position.second].element.second;
        }
    }

    const ValueType& at(const KeyType& key) const {
        // Gets a value at target key

        // Finds target element
        std::pair<bool, size_t> position = find_element_(key);

        // Checks if there is such element
        if (position.first) {
            return _data[position.second].element.second;
        }
        else {
            throw std::out_of_range("There is no target element in HashMap to delete it");
        }
    }

    iterator find(const KeyType& key) {
        // Gets a iterator at target key, if there is none returns end

        // Finds target element
        std::pair<bool, size_t> position = find_element_(key);

        // Checks if there is such element
        if (position.first) {
            return iterator(_data[position.second].ptr);
        }
        else {
            return end();
        }
    }

    const_iterator find(const KeyType& key) const {
        // Gets a const_iterator at target key, if there is none returns end

        // Finds target element
        std::pair<bool, size_t> position = find_element_(key);

        // Checks if there is such element
        if (position.first) {
            return const_iterator(_data[position.second].ptr);
        }
        else {
            return end();
        }
    }

    void clear() {
        // Clears all ADDED elements from structure

        // Clears added elements system info
        _added = 0;

        // Clears added elements & ptrs
        Ptr* ptr = _ptr;
        while (ptr != nullptr) {
            Ptr* next_ptr = ptr->next;
            ptr->cell->validation = Cell::State::EMPTY;
            ptr->cell->ptr = nullptr;

            delete ptr;
            ptr = next_ptr;
        }
        _ptr = nullptr;
    }

    void printOccupancy() const {
        // Prints all validation data & capacity with system values
        // For DEBUG

        std::cout << '[';
        for (size_t i = 0; i < _capacity; ++i) {
            switch (_data[i].validation) {
            case Cell::State::EMPTY:
                std::cout << static_cast<char>(177);
                break;
            case Cell::State::DELETED:
                std::cout << static_cast<char>(178);
                break;
            case Cell::State::FULL:
                std::cout << static_cast<char>(219);
                break;
            default:
                break;
            }
        }
        std::cout << "] capasity: " << _capacity << "; added: "
            << _added << "; deleted: " << _deleted << std::endl;
    }

    void printDeltas() const {
        // Prints all deltas data & capacity with system values
        // For DEBUG

        std::cout << '[';
        for (size_t i = 0; i < _capacity; ++i) {
            switch (_data[i].validation) {
            case Cell::State::EMPTY:
                std::cout << 'E';
                break;
            case Cell::State::DELETED:
                std::cout << 'D';
                break;
            case Cell::State::FULL:
                std::cout << get_delta_(_data[i].hash, i);
                break;
            default:
                break;
            }
            if (i + 1 != _capacity) {
                std::cout << '|';
            }
        }
        std::cout << "] capasity: " << _capacity << "; added: " << _added
            << "; deleted: " << _deleted << std::endl;
    }

    void printElements() const {
        // Prints all deltas data & capacity with system values
        // For DEBUG [NOTE ELEMENTS MAY NOT HAVE PRINTIG OVERLOAD] 

        std::cout << '[';
        for (size_t i = 0; i < _capacity; ++i) {
            switch (_data[i].validation) {
            case Cell::State::EMPTY:
                std::cout << 'E';
                break;
            case Cell::State::DELETED:
                std::cout << 'D';
                break;
            case Cell::State::FULL:
                std::cout << _data[i].element.first << ", " << _data[i].element.second;
                break;
            default:
                break;
            }
            if (i + 1 != _capacity) {
                std::cout << '|';
            }
        }
        std::cout << "] capasity: " << _capacity << "; added: " << _added
            << "; deleted: " << _deleted << std::endl;
    }

    Hash hash_function() const {
        // Returns internal hasher

        return _hasher;
    }

    size_t size() const {
        // Returns number of filled cells

        return _added;
    }

    bool empty() const {
        // Returns is hash table is empty

        return (_added == 0);
    }

private:
    static const size_t          _STARTING_CAPACITY = 128;   // Starting capacity of the HashTable
    static constexpr long double _MAX_FILL_PORTION = 0.75;  // Max portion that can be filled
    static const size_t          _CAPACITY_MULTIPLIER = 2;     // Multiplier for the capacity on resize

    size_t _capacity = 0; // Capacity of the HashTable
    size_t _added = 0; // Number of filled cells (deleted included)
    size_t _deleted = 0; // Number of deleted elements

    Cell* _data = nullptr; // Pointer on internal storage of cells
    Ptr* _ptr = nullptr; // Pointer on first in list of ptrs

    Hash _hasher; // Internal hasher

    void hard_insert_(const std::pair<const KeyType, const ValueType>& element) {
        // Insert element into the map even if there is one with that key

        hard_insert_with_hash_(element, _hasher(element.first));
    }

    void hard_insert_with_hash_(const std::pair<const KeyType, const ValueType>& element, size_t hash) {
        // Insert element into the map even if there is one with that key, not calculating hash

        // Checks if needs to increase capacity
        if (_added + _deleted >= _capacity * _MAX_FILL_PORTION) {
            resize_(_capacity * _CAPACITY_MULTIPLIER);
        }

        // Adds element
        find_place_(hash, hash % _capacity, element);
    }

    size_t get_delta_(size_t hash, size_t position) const {
        // Returns distance from positoin to desired position

        return (position - (hash % _capacity) + _capacity) % _capacity;
    }

    void resize_(size_t new_capacity) {
        // Increases capacity of the table

        // Creates new data with target capacity
        Cell* new_data = nullptr;

        // Try to create new data, in case of memory limit, clears
        try {
            new_data = new Cell[new_capacity];
        }
        catch (std::exception& error) {
            delete[] _data;
            _deletePtrs();

            _capacity = 0;
            _added = 0;
            _deleted = 0;

            throw error;
        }

        // Transfering all elements
        // -Nulling system values
        size_t old_capacity;
        old_capacity = _capacity;
        _capacity = new_capacity;
        _added = 0;
        _deleted = 0;

        // -Swapping datas & clearing old ptrs
        std::swap(_data, new_data);
        _deletePtrs();

        if (new_data != nullptr) {
            // -Insert all from old data
            for (size_t i = 0; i < old_capacity; ++i) {
                if (new_data[i].validation == Cell::State::FULL) {
                    hard_insert_with_hash_(new_data[i].element, new_data[i].hash);
                }
            }

            // Clearing memory
            delete[] new_data;
        }
    }

    void place_(size_t hash_value, size_t position, const std::pair<KeyType, ValueType>& element) {
        // Place element in position

        // Recalculate elements count
        switch (_data[position].validation) {
        case Cell::State::EMPTY:
            _added++;
            break;
        case Cell::State::DELETED:
            _added++;
            _deleted--;
            break;
        case Cell::State::FULL:
            break;
        default:
            break;
        }

        // Filling element data
        _data[position].element = element;
        _data[position].hash = hash_value;
        _data[position].validation = Cell::State::FULL;

        // -Clearing old ptr
        if (_data[position].ptr != nullptr) {
            if (_data[position].ptr->previous != nullptr) {
                _data[position].ptr->previous->next = _data[position].ptr->next;
            }
            if (_data[position].ptr->next != nullptr) {
                _data[position].ptr->next->previous = _data[position].ptr->previous;
            }
            if (_ptr == _data[position].ptr) {
                _ptr = _data[position].ptr->next;
            }
            delete _data[position].ptr;
        }

        // -Creating new ptr, and adding it to the start
        _data[position].ptr = new Ptr(&_data[position]);
        if (_ptr != nullptr) {
            _data[position].ptr->next = _ptr;
            _ptr->previous = _data[position].ptr;
        }
        _ptr = _data[position].ptr;
    }

    void find_place_(size_t hash_value, size_t position, std::pair<KeyType, ValueType> element) {
        // Finds closest valid place to put element

        while (true) {
            switch (_data[position].validation) {
            case Cell::State::EMPTY:
                place_(hash_value, position, element);
                return;
            case Cell::State::DELETED:
                place_(hash_value, position, element);
                return;
            case Cell::State::FULL:
                // Checks if element if cleser to its desired position
                if (get_delta_(hash_value, position) > get_delta_(_data[position].hash, position)) {
                    std::swap(hash_value, _data[position].hash);
                    std::swap(element, _data[position].element);
                }
                break;
            default:
                break;
            }

            (++position) %= _capacity;
        }
    }

    void delete_(size_t position) {
        // Delets element in position

        // Recalculate elements count
        switch (_data[position].validation) {
        case Cell::State::EMPTY:
            return;
        case Cell::State::DELETED:
            return;
        case Cell::State::FULL:
            _added--;
            _deleted++;
            break;
        default:
            break;
        }

        // Filling element data
        _data[position].validation = Cell::State::DELETED;

        // Deleting ptr
        if (_data[position].ptr->previous != nullptr) {
            _data[position].ptr->previous->next = _data[position].ptr->next;
        }
        if (_data[position].ptr->next != nullptr) {
            _data[position].ptr->next->previous = _data[position].ptr->previous;
        }
        if (_ptr == _data[position].ptr) {
            _ptr = _data[position].ptr->next;
        }
        delete _data[position].ptr;
        _data[position].ptr = nullptr;
    }

    std::pair<bool, size_t> find_element_(const KeyType& key) const {
        // Finds element
        // Returns pair (if found \ position)

        return find_element_with_hash_(key, _hasher(key));
    }

    std::pair<bool, size_t> find_element_with_hash_(const KeyType& key, size_t hash) const {
        // Finds element, not calculating hash
        // Returns pair (if found \ position)

        size_t position = hash % _capacity;

        while (true) {
            switch (_data[position].validation) {
            case Cell::State::EMPTY:
                return std::make_pair(false, position);
            case Cell::State::DELETED:
                break;
            case Cell::State::FULL:
                if (_data[position].element.first == key) {
                    return std::make_pair(true, position);
                }
                break;
            default:
                break;
            }

            (++position) %= _capacity;
        }
    }

    void _deletePtrs() {
        // Deletes all ptrs memory

        Ptr* ptr = _ptr;
        while (ptr != nullptr) {
            Ptr* next_ptr = ptr->next;
            ptr->cell->ptr = nullptr;

            delete ptr;
            ptr = next_ptr;
        }
        _ptr = nullptr;
    }
};