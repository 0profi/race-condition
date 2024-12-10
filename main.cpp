#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include <optional>

// Глобальный вектор для хранения чисел
std::vector<int> global_vector;
// Мьютекс для синхронизации доступа к вектору
std::mutex vector_mutex;
// Флаг для управления работой потоков
bool running = true;

class Generator {
public:
    Generator() : gen(std::random_device{}()), dist(0, 100) {}

    void start() {
        generator_thread = std::thread(&Generator::generate, this);
    }

    void stop() {
        running = false;
        if (generator_thread.joinable()) {
            generator_thread.join();
        }
    }

    ~Generator() {
        stop();
    }

private:
    std::thread generator_thread;
    std::mt19937 gen; // Генератор случайных чисел
    std::uniform_int_distribution<int> dist; // Диапазон случайных чисел

    void generate() {
        while (running) {
            int value = dist(gen);

            vector_mutex.lock();
            global_vector.push_back(value);
            vector_mutex.unlock();

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

class Consumer {
public:
    Consumer(int id) : consumer_id(id) {}

    void start() {
        consumer_thread = std::thread(&Consumer::consume, this);
    }

    void stop() {
        running = false;
        if (consumer_thread.joinable()) {
            consumer_thread.join();
        }
    }

    ~Consumer() {
        stop();
    }

private:
    int consumer_id;
    std::thread consumer_thread;

    void consume() {
        while (running) {
            std::optional<int> value = get_value();

            if (value.has_value()) {
                process_value(value.value());
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Если вектор пуст, ждем
            }
        }
    }

    std::optional<int> get_value() {
        vector_mutex.lock();
        if (!global_vector.empty()) {
            int value = global_vector.front();
            global_vector.erase(global_vector.begin());
            vector_mutex.unlock();
            return value;
        }
        vector_mutex.unlock();
        return std::nullopt; // Вектор пуст, возвращаем "ничего"
    }

    void process_value(int value) {
        std::cout << "Потребитель " << consumer_id << " обрабатывает число: " << value << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Обработка числа
    }
};

int main() {
    Generator generator;
    generator.start();

    // Создаём три объекта Consumer
    Consumer consumer1(1);
    Consumer consumer2(2);
    Consumer consumer3(3);

    consumer1.start();
    consumer2.start();
    consumer3.start();

    std::cout << "Генератор и потребители запущены. Нажмите Enter для завершения программы...\n";
    std::cin.get();

    generator.stop();
    consumer1.stop();
    consumer2.stop();
    consumer3.stop();

    std::cout << "Оставшиеся числа в векторе:\n";
    vector_mutex.lock();
    for (int num : global_vector) {
        std::cout << num << " ";
    }
    vector_mutex.unlock();
    std::cout << std::endl;

    return 0;
}
