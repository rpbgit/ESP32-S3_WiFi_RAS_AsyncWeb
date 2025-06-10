#ifndef STR_RING_BUFFER_H
#define STR_RING_BUFFER_H 1

#include <cstring> // For strcpy, strcat

constexpr size_t BUFFER_SIZE = 40;    // Number of slots in the buffer
constexpr size_t MAX_STRING_LEN = 200;  // Max length of each string

class StringRingBuffer {
public:
    StringRingBuffer() : head(0), tail(0), full(false) {
        // Initialize the buffer with empty strings
        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            buffer[i][0] = '\0';
        }
    }

    // Put a string into the ring buffer (overwrites oldest if full)
    void put(const char* str) {
        strncpy(buffer[head], str, MAX_STRING_LEN - 1);
        buffer[head][MAX_STRING_LEN - 1] = '\0'; // Ensure null-termination
        if (full) {
            tail = (tail + 1) % BUFFER_SIZE; // Move tail forward to overwrite
        }
        head = (head + 1) % BUFFER_SIZE;
        full = head == tail;
    }

    // Get the string from the tail and move the tail forward
    const char* get() {
        if (isEmpty()) {
            return nullptr;
        }

        const char* result = buffer[tail];
        tail = (tail + 1) % BUFFER_SIZE;
        full = false;
        return result;
    }

    // Concatenate all strings in the buffer into a single string
    const char* concat_all(char* result)  {
        result[0] = '\0';  // Initialize as empty string
        size_t index = tail;
        while (index != head || (full && index == head)) {
            strcat(result, buffer[index]);
            strcat(result, " "); // Add space between strings
            index = (index + 1) % BUFFER_SIZE;
            //if (!full && index == head) break;
            if(index == head) break;
        }
        return result;
    }

    // Delete all strings in the buffer
    void delete_all() {
        head = tail;
        full = false;
        // Optionally, clear the buffer's content
        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            buffer[i][0] = '\0';
        }
    }

    bool isEmpty() const {
        return !full && head == tail;
    }

private:
    char buffer[BUFFER_SIZE][MAX_STRING_LEN]; // Statically allocated buffer
    size_t head, tail;
    bool full;
};
#endif

#if 0
//////////////////////////////////////////////////////////////////////////
int main() {
	StringRingBuffer ring;
	char catbuf[500] = {0};

	ring.put(":one");
	ring.put(":two");
	ring.put(":three");

	ring.concat_all(catbuf);
	std::cout << "concat after 3: " << catbuf << std::endl;
	*catbuf = 0;

	ring.delete_all();
	ring.put(":four");
	ring.put(":five_max");
	ring.put(":six");

	ring.concat_all(catbuf);
	std::cout << "concat after delete and add 4,5,6: " << catbuf << std::endl;
	*catbuf = 0;

	ring.put(":seven");
	while (!ring.isEmpty()) {
		cout << "Get: " << ring.get() << std::endl;
	}
	ring.concat_all(catbuf);
	std::cout << "concat after adding 7 and get all - expect null : " << catbuf << std::endl;
	*catbuf = 0;

	while (!ring.isEmpty()) {
		std::cout << "Get: " << ring.get() << std::endl;
	}

	ring.put("eight");
	ring.concat_all(catbuf);
	std::cout << "Concatenated 3 - expect 8 : " << catbuf << std::endl;
	*catbuf = 0;

	ring.delete_all();

	return 0;
}

#endif