#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>

struct Vec2 {
    float x, y;
};

Vec2 Lerp(const Vec2& a, const Vec2& b, float alpha) {
    return { a.x * (1.0f - alpha) + b.x * alpha, a.y * (1.0f - alpha) + b.y * alpha };
}

struct Car {
    Vec2 position;
    Vec2 previousPosition;
    Vec2 velocity; 
    Vec2 acceleration; 
    float maxSpeed; 
    float damping; 
    sf::RectangleShape shape; 

    Car(Vec2 pos, Vec2 vel, sf::Color color, float maxSpeed_, float damping_) {
        position = pos;
        previousPosition = pos;
        velocity = vel;
        acceleration = { 0.f, 0.f };
        maxSpeed = maxSpeed_;
        damping = damping_;
        shape.setSize(sf::Vector2f(100.f, 45.f));
        shape.setFillColor(color);
        shape.setOrigin(50.f, 22.5f); 
    }

    void update(float dt) {
        previousPosition = position;
        velocity.x += acceleration.x * dt; 
        velocity.y += acceleration.y * dt;

        // Sürtünme
        if (acceleration.x == 0.f && acceleration.y == 0.f) {
            velocity.x *= damping;
            velocity.y *= damping;
        }

        // Hýz Sýnýrý
        float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y); 
        if (speed > maxSpeed) {
            velocity.x = (velocity.x / speed) * maxSpeed; 
            velocity.y = (velocity.y / speed) * maxSpeed;
        }

        position.x += velocity.x * dt; 
        position.y += velocity.y * dt;
        shape.setPosition(position.x, position.y);
    }

    bool checkCollision(const Car& other) const {
        return shape.getGlobalBounds().intersects(other.shape.getGlobalBounds()); 
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Highway Racer - Realism Update"); 
    window.setFramerateLimit(144); 
    sf::View camera(sf::FloatRect(0.f, .0f, 1920.f, 1080.f));      

    std::random_device rd; 
    std::mt19937 gen(rd());

    float lanesY[6] = { 150.f, 270.f, 390.f, 690.f, 810.f, 930.f };
    std::uniform_int_distribution<> laneDist(0, 5);
    std::uniform_real_distribution<float> speedDist(250.f, 500.f);

    const float fixedDt = 1.0f / 60.0f; 
    float accumulator = 0.0f; 
    sf::Clock clock;

    Car player({ 300.f, 810.f }, { 0.f, 0.f }, sf::Color::Red, 600.f, 0.96f);
    float playerAccelX = 600.f;
    float playerAccelY = 500.f;

    std::vector<Car> traffic;
    for (int i = 0; i < 28; ++i) { 
        int laneIndex = laneDist(gen); 
        float spawnY = lanesY[laneIndex]; 
        float spawnX = static_cast<float>((gen() % 4000) - 1000); 
        float speed = speedDist(gen); 
        float dirX = (laneIndex <= 2) ? -speed : speed; 

        traffic.emplace_back(Vec2{ spawnX, spawnY }, Vec2{ dirX, 0.f }, sf::Color::Blue, 450.f, 1.0f); 
    }

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) window.close();
        }

        float frameTime = clock.restart().asSeconds();
        if (frameTime > 0.25f) frameTime = 0.25f;
        accumulator += frameTime;

        player.acceleration = { 0.f, 0.f }; 
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) player.acceleration.y = -playerAccelY; 
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) player.acceleration.y = playerAccelY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) player.acceleration.x = -playerAccelX;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) player.acceleration.x = playerAccelX;

        while (accumulator >= fixedDt) { 
            float targetCamx = player.position.x + 600.f; 
            camera.setCenter(targetCamx, 540.f); 
            float viewLeft = camera.getCenter().x - 960.f; 
            float viewRight = camera.getCenter().x + 960.f; 
            
            if (player.position.y > 450.f && player.position.y < 630.f) { 
                if (player.velocity.y > 0) player.position.y = 450.f; else player.position.y = 630.f; 
                player.velocity.y = 0.f; 
            }
            if (player.position.y < 50.f) player.position.y = 50.f; 
            if (player.position.y > 1030.f) player.position.y = 1030.f; 

            for (size_t i = 0; i < traffic.size(); ++i) { 
                bool slowDown = false; 
                float targetSpeed = 0.f; 
                float minDistance = 300.f; 

                for (size_t j = 0; j < traffic.size(); ++j) { 
                    if (i == j) continue; 
                    if (std::abs(traffic[i].position.y - traffic[j].position.y) < 20.f) { 
                        float dx = traffic[j].position.x - traffic[i].position.x; 
                        bool inFrontRight = (traffic[i].velocity.x > 0 && dx > 0 && dx < minDistance); 
                        bool inFrontLeft = (traffic[i].velocity.x < 0 && dx < 0 && dx > -minDistance); 

                        if (inFrontRight || inFrontLeft) { 
                            slowDown = true; 
                            targetSpeed = traffic[j].velocity.x; 
                            minDistance = std::abs(dx); 
                        }
                    }
                }



                if (std::abs(traffic[i].position.y - player.position.y) < 25.f) { 
                    float dx = player.position.x - traffic[i].position.x; 

                    bool playerInFrontRight = (traffic[i].velocity.x > 0 && dx > 0 && dx < minDistance); 
                    bool playerInFrontLeft = (traffic[i].velocity.x < 0 && dx < 0 && dx > -minDistance); 

                    if (playerInFrontRight || playerInFrontLeft) { 
                        slowDown = true; 
                        targetSpeed = player.velocity.x;
                    }
                }

                if (slowDown) { 
                    if (traffic[i].velocity.x > 0) { 
                        traffic[i].velocity.x -= 600.f * fixedDt; 

                        if (traffic[i].velocity.x < targetSpeed) { 
                            traffic[i].velocity.x = targetSpeed; 
                        }

                        if (traffic[i].velocity.x < 0.f) traffic[i].velocity.x = 0.f; 
                    }
                    else { 
                        traffic[i].velocity.x += 600.f * fixedDt;

                        if (traffic[i].velocity.x > targetSpeed) {
                            traffic[i].velocity.x = targetSpeed;
                        }

                        if (traffic[i].velocity.x > 0.f) traffic[i].velocity.x = 0.f; 
                    }
                }
                else { 
                    float myMax = (traffic[i].velocity.x > 0) ? traffic[i].maxSpeed : -traffic[i].maxSpeed;

                    if (traffic[i].velocity.x >= 0) { 
                        if (traffic[i].velocity.x < traffic[i].maxSpeed) 
                            traffic[i].velocity.x += 200.f * fixedDt; 
                    }
                    else { 
                        if (traffic[i].velocity.x > -traffic[i].maxSpeed) 
                            traffic[i].velocity.x -= 200.f * fixedDt;
                    }
                }

                traffic[i].update(fixedDt); 

                if (traffic[i].position.x < viewLeft - 200.f) {
                    traffic[i].position.x = viewRight + (gen() % 500 + 200);
                    traffic[i].previousPosition = traffic[i].position;
                    int laneIndex = laneDist(gen); 
                    traffic[i].position.y = lanesY[laneIndex];
                    float speed = speedDist(gen); 
                    traffic[i].velocity.x = (laneIndex <= 2) ? -speed : speed; 
                }
                else if (traffic[i].velocity.x > 0 && traffic[i].position.x > viewRight + 2000.f) { 
                    traffic[i].position.x = viewLeft - 200.f; 
                    traffic[i].previousPosition = traffic[i].position;
                    int laneIndex = 3 + (laneDist(gen) % 3);
                    traffic[i].position.y = lanesY[laneIndex];
                    traffic[i].velocity.x = speedDist(gen);
                }
            }

            player.update(fixedDt); 

            for (auto& car : traffic) {
                if (player.checkCollision(car)) { 
                    player.velocity.x *= 0.8f; 
                    if (std::abs(player.velocity.x) > 10.f) 
                        player.position = player.previousPosition; 
                }
            }
            accumulator -= fixedDt; 
        }

        float alpha = accumulator / fixedDt; 
        window.clear(sf::Color(30, 30, 30)); 
        Vec2 pPos = Lerp(player.previousPosition, player.position, alpha);
        float smoothCamX = pPos.x + 600.f;
        camera.setCenter(smoothCamX, 540.f);
        window.setView(camera);
        float viewLeft = smoothCamX- 960.f; 
        float viewRight = smoothCamX+ 960.f;

        sf::RectangleShape barrier(sf::Vector2f(viewRight-viewLeft, 140.f)); 
        barrier.setPosition(viewLeft, 490.f); 
        barrier.setFillColor(sf::Color(90, 90, 90)); 
        barrier.setOutlineThickness(4.f); 
        barrier.setOutlineColor(sf::Color(200, 180, 0)); 
        window.draw(barrier); 

        sf::RectangleShape line(sf::Vector2f(50.f, 5.f)); 
        line.setFillColor(sf::Color(200, 200, 200, 150)); 
        int startLineIndex = (int)(viewLeft / 150.f) - 1; 
        int endLineIndex = (int)(viewRight / 150.f) + 1; 
        for (int i = startLineIndex; i < endLineIndex; ++i) { 
            float lineX = i * 150.f; 
            line.setPosition(lineX, 210.f); window.draw(line); 
            line.setPosition(lineX, 330.f); window.draw(line);
            line.setPosition(lineX, 750.f); window.draw(line);
            line.setPosition(lineX, 870.f); window.draw(line);
        }

        for (auto& car : traffic) {
            Vec2 rPos = Lerp(car.previousPosition, car.position, alpha);
            car.shape.setPosition(rPos.x, rPos.y);
            window.draw(car.shape);
        }

        player.shape.setPosition(pPos.x, pPos.y);
        window.draw(player.shape);

        window.display();
    }
    return 0;
}