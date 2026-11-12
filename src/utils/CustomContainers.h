#ifndef CUSTOM_CONTAINERS_H
#define CUSTOM_CONTAINERS_H

#include <stdexcept>

// Simple Node for Linked List
template <typename T>
struct Node {
    T data;
    Node* next;
    Node(T val) : data(val), next(nullptr) {}
};

// Custom Stack using Linked List
template <typename T>
class Stack {
private:
    Node<T>* topNode;
    int count;

public:
    Stack() : topNode(nullptr), count(0) {}
    ~Stack() {
        while (!isEmpty()) {
            pop();
        }
    }

    void push(T val) {
        Node<T>* newNode = new Node<T>(val);
        newNode->next = topNode;
        topNode = newNode;
        count++;
    }

    void pop() {
        if (isEmpty()) return;
        Node<T>* temp = topNode;
        topNode = topNode->next;
        delete temp;
        count--;
    }

    T top() {
        if (isEmpty()) throw std::runtime_error("Stack is empty");
        return topNode->data;
    }

    bool isEmpty() const {
        return topNode == nullptr;
    }

    int size() const {
        return count;
    }
};

// Custom Queue using Linked List
template <typename T>
class Queue {
private:
    Node<T>* frontNode;
    Node<T>* rearNode;
    int count;

public:
    Queue() : frontNode(nullptr), rearNode(nullptr), count(0) {}
    ~Queue() {
        while (!isEmpty()) {
            pop();
        }
    }

    void push(T val) {
        Node<T>* newNode = new Node<T>(val);
        if (isEmpty()) {
            frontNode = rearNode = newNode;
        } else {
            rearNode->next = newNode;
            rearNode = newNode;
        }
        count++;
    }

    void pop() {
        if (isEmpty()) return;
        Node<T>* temp = frontNode;
        frontNode = frontNode->next;
        if (frontNode == nullptr) {
            rearNode = nullptr;
        }
        delete temp;
        count--;
    }

    T front() {
        if (isEmpty()) throw std::runtime_error("Queue is empty");
        return frontNode->data;
    }

    bool isEmpty() const {
        return frontNode == nullptr;
    }

    int size() const {
        return count;
    }
};

#endif
