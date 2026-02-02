#include <Arduino.h>
// #include <Adafruit_GFX.h>  
#include <SPI.h>
#include "TFT_eSPI.h"
#include "WS_QMI8658.h"

#define SNAKE_MAX_LENGTH 500     // Define the maximum length of the snake
#define MAP_WIDTH 240            // Define the map width
#define MAP_HEIGHT 240           // Define the map height
#define GRID_SIZE 10             // Define the size of each grid, the map is divided into a 24x24 grid

TFT_eSPI tft = TFT_eSPI(MAP_WIDTH, MAP_HEIGHT); // Initialize the TFT display object

// Define the direction of the snake
typedef enum { STOP = 0,
               LEFT,
               RIGHT,
               UP,
               DOWN } Direction;

Direction currentDirection = STOP; // Current direction of the snake
Direction lastDirection = STOP;    // Previous direction

// Add a flag to track whether the snake has eaten food
bool hasEatenFood = false;

// Snake body array
typedef struct {
  int x;
  int y;
} Segment;

Segment snakeBody[SNAKE_MAX_LENGTH]; // Snake body array
int snakeLength = 3;                 // Current length of the snake
int snakeHeadIndex = 0;              // Index of the snake's head

// Position of the food
int foodX, foodY;
bool foodExists = false;             // Flag indicating whether food exists

// Initialize food position
void spawnFood() {
  foodExists = true;                 // Set the food existence flag
  foodX = (random() % (MAP_WIDTH / GRID_SIZE)) * GRID_SIZE; // Randomly generate food X coordinate
  foodY = (random() % (MAP_HEIGHT / GRID_SIZE)) * GRID_SIZE; // Randomly generate food Y coordinate

  // Ensure that the food is not placed on the snake
  for (int i = 0; i < snakeLength; i++) {
    int segmentIndex = (snakeHeadIndex + i) % snakeLength;
    if (snakeBody[segmentIndex].x == foodX && snakeBody[segmentIndex].y == foodY) {
      spawnFood();  // If the food is on the snake, regenerate it
      break;
    }
  }
}

// Function to reset the snake's position and length
void resetSnake() {
  for (int i = 0; i < SNAKE_MAX_LENGTH; i++) {
    snakeBody[i].x = 0;
    snakeBody[i].y = 0;
  }
  // Initialize the snake's position and length
  for (int i = 0; i < 12; i++) {
    snakeBody[i].x = MAP_WIDTH / 2 - i * GRID_SIZE;
    snakeBody[i].y = MAP_HEIGHT / 2;
  }
  snakeLength = 12; // Set the initial length of the snake to 12
}

// Update the snake's position
void updateSnakePosition() {
  // Determine the head position
  int newHeadX = snakeBody[snakeHeadIndex].x;
  int newHeadY = snakeBody[snakeHeadIndex].y;

  switch (currentDirection) {
    case LEFT: newHeadX -= GRID_SIZE; break;
    case RIGHT: newHeadX += GRID_SIZE; break;
    case UP: newHeadY -= GRID_SIZE; break;
    case DOWN: newHeadY += GRID_SIZE; break;
    default: break;
  }

  // Apply wrap-around boundary conditions
  newHeadX = (newHeadX + MAP_WIDTH) % MAP_WIDTH;    // X-axis wrap-around
  newHeadY = (newHeadY + MAP_HEIGHT) % MAP_HEIGHT;  // Y-axis wrap-around

  // Check if the snake collides with itself
  bool hitSelf = false;
  for (int i = 1; i < snakeLength; i++) {
    int segmentIndex = (snakeHeadIndex + i) % snakeLength;
    if (snakeBody[segmentIndex].x == newHeadX && snakeBody[segmentIndex].y == newHeadY) {
      hitSelf = true;
      break;
    }
  }

  if (hitSelf) {
    resetSnake(); // Reset the snake if it collides with itself
    spawnFood();  // Regenerate food
    return;
  }

  // Check if the snake eats food and handle it
  bool grew = false;
  if (newHeadX == foodX && newHeadY == foodY) {
    foodExists = false;
    snakeLength++;
    hasEatenFood = true;
    spawnFood();
    grew = true;  // Mark that the snake has grown
  }

  // Update the snake's body position
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeBody[i] = snakeBody[i - 1];
  }
  snakeBody[0] = { newHeadX, newHeadY }; // Update the snake's head position
}

// Draw the snake and food
void draw() {
  // Fill the screen with black
  tft.fillScreen(TFT_BLACK);

  // Draw the snake's head
  int headIndex = snakeHeadIndex % snakeLength; // Get the head index (always 0, but for consistency)
  tft.fillRect(snakeBody[headIndex].x, snakeBody[headIndex].y, GRID_SIZE, GRID_SIZE, TFT_BLUE);

  // Draw the rest of the snake's body
  for (int i = 1; i < snakeLength; i++) { // Start from 1 since the head is already drawn
    int segmentIndex = (snakeHeadIndex + i) % snakeLength; // Use circular indexing for the snake's body
    tft.fillRect(snakeBody[segmentIndex].x, snakeBody[segmentIndex].y, GRID_SIZE, GRID_SIZE, TFT_GREEN);
  }

  if (foodExists) {
    tft.fillRect(foodX, foodY, GRID_SIZE, GRID_SIZE, TFT_RED);
  }
}

// Update direction using the gyroscope
void updateDirectionFromGyro() {
  float accelX = QMI8658_get_A_fx();  // Read X-axis acceleration
  float accelY = QMI8658_get_A_fy();  // Read Y-axis acceleration

  static unsigned long lastChangeTime = 0;  // Last time the direction changed
  unsigned long currentTime = millis();     // Current time

  // Debounce time interval
  if (currentTime - lastChangeTime > 200) {
    // Check if acceleration is below a threshold, indicating a stable horizontal position
    float accelMagnitude = sqrt(accelX * accelX + accelY * accelY);
    if (accelMagnitude < 1.0) {  // Assume 1.0 is the threshold for stability, adjust as needed
                                 // If acceleration is small, maintain current direction
                                 // No direction update
    } else {
      Direction newDirection = STOP;
      if (abs(accelX) > abs(accelY)) {
        newDirection = (accelX > 0) ? RIGHT : LEFT;
      } else {
        newDirection = (accelY > 0) ? UP : DOWN;
      }
      // Prevent immediate reverse direction
      if ((newDirection == LEFT && lastDirection == RIGHT) || (newDirection == RIGHT && lastDirection == LEFT) ||
          (newDirection == UP && lastDirection == DOWN) || (newDirection == DOWN && lastDirection == UP)) {
        newDirection = currentDirection;  // Maintain current direction
      } else {
        currentDirection = newDirection;  // Update direction
      }
      lastDirection = currentDirection;  // Update last direction
    }
    lastChangeTime = currentTime;  // Update last direction change time
  }
}

void setup() {
  Serial.begin(115200);
  tft.begin();
  SPI.begin();
  QMI8658_Init();

  // Initialize the snake's position and length
  for (int i = 0; i < 12; i++) {
    snakeBody[i].x = MAP_WIDTH / 2 - i * GRID_SIZE;
    snakeBody[i].y = MAP_HEIGHT / 2;
  }
  snakeLength = 12; // Set the initial length of the snake to 12
  snakeHeadIndex = 0; // The head index is always 0
  spawnFood();  // Generate food for the first time
}

void loop() {
  Serial.print("Snake Length: ");
  Serial.println(snakeLength);  // Debug output showing the snake's length
  updateDirectionFromGyro();
  updateSnakePosition();
  draw();
  delay(500);
}
