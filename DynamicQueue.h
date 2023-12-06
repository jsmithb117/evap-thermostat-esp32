#include <Arduino.h>

#ifndef Message
struct Message {
  const char* topic;
  const char* payload;
};
#endif

class DynamicQueue {
  private:
  Message* queue;
  int capacity;
  int front;
  int rear;
  char** subscribeList;
  int subscribeCount;

  void resizeQueue() {
    int newCapacity = capacity * 2;
    Message* newQueue = new Message[newCapacity];

    int i = 0;
    int j = front;
    while (j != (rear + 1) % capacity) {
      // Deep copy strings to avoid memory issues
      newQueue[i].topic = strdup(queue[j].topic);
      newQueue[i].payload = strdup(queue[j].payload);

      j = (j + 1) % capacity;
      i++;
    }

    front = 0;
    rear = i - 1;
    capacity = newCapacity;

    // Free memory of the old queue
    for (int k = 0; k < i; k++) {
      free(const_cast<char*>(queue[k].topic));
      free(const_cast<char*>(queue[k].payload));
    }
    delete[] queue;

    // Assign the new queue
    queue = newQueue;
  }

  public:
  DynamicQueue() // constructor
      : capacity(10),
        front(-1),
        rear(-1),
        subscribeList(nullptr),
        subscribeCount(0) {
    queue = new Message[capacity];
  }

  ~DynamicQueue() { // destructor
    // Free memory of the entire queue
    for (int i = front; i != (rear + 1) % capacity; i = (i + 1) % capacity) {
      free(const_cast<char*>(queue[i].topic));
      free(const_cast<char*>(queue[i].payload));
    }
    delete[] queue;

    // Free memory of the subscribeList
    for (int i = 0; i < subscribeCount; i++) {
      free(subscribeList[i]);
    }
    delete[] subscribeList;
  }

  void enqueue(const char* topic, const char* payload) {
    if ((rear + 1) % capacity == front) {
      // Queue is full, resize
      resizeQueue();
    }

    if (front == -1) {
      // Empty queue
      front = 0;
    }

    // Deep copy strings to avoid memory issues
    queue[rear + 1].topic = strdup(topic);
    queue[rear + 1].payload = strdup(payload);

    rear = (rear + 1) % capacity;
  }

  Message dequeue() {
    Message emptyMessage = {nullptr, nullptr};

    if (front != -1) {
      // Deep copy strings to avoid memory issues
      Message result;
      result.topic = strdup(queue[front].topic);
      result.payload = strdup(queue[front].payload);

      if (front == rear) {
        // Last element in the queue
        front = rear = -1;
      } else {
        front = (front + 1) % capacity;
      }

      return result;
    }

    return emptyMessage;
  }

  bool isEmpty() { return front == -1; }

  void addSubscription(const char* topic) {
    // Add a topic to the subscribeList if not exists
    for (int i = 0; i < subscribeCount; i++) {
      if (strcmp(subscribeList[i], topic) == 0) {
        // Topic is already in the list, don't add it again
        return;
      }
    }

    // Add the topic to the subscribeList
    char* newTopic = strdup(topic);
    char** newSubscribeList = new char*[subscribeCount + 1];

    for (int i = 0; i < subscribeCount; i++) {
      newSubscribeList[i] = subscribeList[i];
    }

    newSubscribeList[subscribeCount] = newTopic;
    subscribeCount++;

    delete[] subscribeList;
    subscribeList = newSubscribeList;
  }

  void removeSubscription(const char* topic) {
    // Remove the topic from the subscribeList
    for (int i = 0; i < subscribeCount; i++) {
      if (strcmp(subscribeList[i], topic) == 0) {
        // Found the topic, remove it from the list
        free(subscribeList[i]);

        // Shift remaining elements in the array
        for (int j = i; j < subscribeCount - 1; j++) {
          subscribeList[j] = subscribeList[j + 1];
        }

        subscribeCount--;

        // Resize the array
        char** newSubscribeList = new char*[subscribeCount];
        for (int j = 0; j < subscribeCount; j++) {
          newSubscribeList[j] = subscribeList[j];
        }

        delete[] subscribeList;
        subscribeList = newSubscribeList;

        break;  // Topic found and removed, exit the loop
      }
    }
  }
  char** getSubscribeList(int* count) {
    *count = subscribeCount;
    return subscribeList;
  }
};

// void setup() {
//     Serial.begin(9600);

//     // Create a DynamicQueue
//     DynamicQueue myQueue;

//     // Testing the dynamic queue with struct elements
//     Serial.println("Is the queue empty? " + String(myQueue.isEmpty()));  //
//     Should print "Is the queue empty? 1" (true)

//     myQueue.enqueue("Topic1", "Message1");
//     Serial.println("Is the queue empty? " + String(myQueue.isEmpty()));  //
//     Should print "Is the queue empty? 0" (false)

//     Message msg1 = myQueue.dequeue();
//     Serial.print("Dequeued: Topic=");
//     Serial.print(msg1.topic);
//     Serial.print(", Message=");
//     Serial.println(msg1.payload);

//     Serial.println("Is the queue empty? " + String(myQueue.isEmpty()));  //
//     Should print "Is the queue empty? 1" (true)

//     // Add subscriptions
//     myQueue.addSubscription("Topic2");
//     myQueue.addSubscription("Topic3");

//     // Remove a subscription
//     myQueue.removeSubscription("Topic2");

//     // Display remaining subscriptions
//     Serial.print("Remaining subscriptions: ");
//     for (int i = 0; i < myQueue.subscribeCount; i++) {
//         Serial.print(myQueue.subscribeList[i]);
//         Serial.print(" ");
//     }
//     Serial.println();
// }

// void loop() {

// }
