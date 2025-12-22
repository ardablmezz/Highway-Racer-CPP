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

        if (acceleration.x == 0.f && acceleration.y == 0.f) {
            velocity.x *= damping;
            velocity.y *= damping;
        }

        float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (speed > maxSpeed) {
            velocity.x = (velocity.x / speed) * maxSpeed;
            velocity.y = (velocity.y / speed) * maxSpeed;
        }

        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        shape.setPosition(position.x, position.y);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Highway Racer - Smart Traffic");
    window.setFramerateLimit(144);
    sf::View camera(sf::FloatRect(0.f, .0f, 1920.f, 1080.f));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> speedDist(250.f, 500.f);

    float lanesY[6] = { 240.f, 360.f, 480.f, 610.f, 730.f, 845.f };
    const float fixedDt = 1.0f / 60.0f;
    float accumulator = 0.0f;
    sf::Clock clock;

    Car player({ 300.f, 810.f }, { 0.f, 0.f }, sf::Color::Red, 600.f, 0.96f);
    float playerAccelX = 600.f;
    float playerAccelY = 500.f;

    std::vector<Car> traffic;

    for (int i = 0; i < 50; ++i) {
        int laneIndex;
        float dirX;
        if (i < 23) {
			laneIndex = std::uniform_int_distribution<>(0, 2)(gen);
			dirX = -speedDist(gen);
        }
        else {
			laneIndex = std::uniform_int_distribution<>(3, 5)(gen);
			dirX = speedDist(gen);
        }

		float spawnX = static_cast<float>((gen() % 15000) - 5000);
        traffic.emplace_back(Vec2{ spawnX, lanesY[laneIndex] }, Vec2{ dirX, 0.f }, sf::Color::Blue, 450.f, 1.0f);
    }

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) { if (event.type == sf::Event::Closed) window.close(); }

        float frameTime = clock.restart().asSeconds();
        if (frameTime > 0.25f) frameTime = 0.25f;
        accumulator += frameTime;

        player.acceleration = { 0.f, 0.f };
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) player.acceleration.y = -playerAccelY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) player.acceleration.y = playerAccelY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) player.acceleration.x = -playerAccelX;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) player.acceleration.x = playerAccelX;

        while (accumulator >= fixedDt) {
            camera.setCenter(player.position.x + 600.f, 540.f);
			float vL = camera.getCenter().x - 1200.f; 
			float vR = camera.getCenter().x + 1200.f; 

            
			if (player.position.y < 205.f) { 
                player.position.y += 3.5f; player.velocity.y = std::max(0.f, player.velocity.y + 120.f) * 0.5f; 
            } 
			if (player.position.y > 875.f) { 
                player.position.y -= 3.5f; player.velocity.y = std::min(0.f, player.velocity.y - 120.f) * 0.5f; 
            }

            for (size_t i = 0; i < traffic.size(); ++i) {
                float targetSpeed = 450.f;
                bool brake = false, gas = false;
                float currentAbs = std::abs(traffic[i].velocity.x);

                
                for (size_t j = 0; j < traffic.size(); ++j) {
                    if (i == j) continue;
                    if (std::abs(traffic[i].position.y - traffic[j].position.y) < 25.f) {
                        float dx = traffic[j].position.x - traffic[i].position.x;
                        bool isAhead = (traffic[i].velocity.x * dx > 0);
                        float dist = std::abs(dx);
                        if (isAhead && dist < 400.f) {
                            if (dist < 180.f) {
                                traffic[i].position.x = traffic[j].position.x - (dx > 0 ? 180.f : -180.f);
                                traffic[i].velocity.x = traffic[j].velocity.x;
                                brake = true;
                            }
                            else {
                                targetSpeed = std::abs(traffic[j].velocity.x) * 0.95f;
                                if (currentAbs > targetSpeed) brake = true;
                            }
                        }
                    }
                }

                
                if (std::abs(traffic[i].position.y - player.position.y) < 25.f) {
                    float dx = player.position.x - traffic[i].position.x;
                    if (traffic[i].velocity.x * dx > 0) {
                        float radar = (traffic[i].velocity.x > 0) ? 350.f : 500.f;
                        if (std::abs(dx) < radar) {
                            if (traffic[i].velocity.x > 0) {
                                if (currentAbs < std::abs(player.velocity.x)) gas = true; else brake = true;
                            }
                            else brake = true;
                        }
                    }
                }

                if (brake) {
                    traffic[i].velocity.x *= 0.95f;
                }

                else if (gas || currentAbs < 450.f) {
                    float acc = (traffic[i].velocity.x >= 0) ? 220.f : -220.f;
                    traffic[i].velocity.x += acc * fixedDt;
                }
                traffic[i].update(fixedDt);

                
                if (traffic[i].position.x < vL - 1500.f || traffic[i].position.x > vR + 3500.f) {
                    
                    bool movingRight = (traffic[i].velocity.x > 0);
                    float newX = movingRight ? vL - 1000.f : vR + 3000.f;

                    
                    int newLane = movingRight ? std::uniform_int_distribution<>(3, 5)(gen) : std::uniform_int_distribution<>(0, 2)(gen);

                    
                    bool blocked = false;
                    for (size_t k = 0; k < traffic.size(); ++k) {
                        if (i == k) continue;
                        
                        if (std::abs(traffic[k].position.y - lanesY[newLane]) < 25.f &&
                            std::abs(traffic[k].position.x - newX) < 600.f) {
                            blocked = true;
                            break;
                        }
                    }

                    if (!blocked) {
                        traffic[i].position.x = newX;
                        traffic[i].position.y = lanesY[newLane];
                        traffic[i].previousPosition = traffic[i].position;
                    }
                }
            }

            player.update(fixedDt);

            
            for (auto& npc : traffic) {
                sf::FloatRect pB = player.shape.getGlobalBounds();
                sf::FloatRect nB = npc.shape.getGlobalBounds();
                sf::FloatRect overlap;
                if (pB.intersects(nB, overlap)) {
                    if (overlap.width < overlap.height) {
                        player.position.y += (player.position.y < npc.position.y) ? -1.1f : 1.1f;
                        player.velocity.y *= 0.75f; player.velocity.x *= 0.995f;
                    }
                    else {
                        player.position.x = npc.position.x + (player.position.x < npc.position.x ? -105.f : 105.f);
                        player.velocity.x = npc.velocity.x * (player.position.x < npc.position.x ? 0.7f : 1.1f);
                    }
                }
            }
            accumulator -= fixedDt;
        }

        
        float alpha = accumulator / fixedDt;
        window.clear(sf::Color(30, 30, 30));
        Vec2 pPos = Lerp(player.previousPosition, player.position, alpha);
        camera.setCenter(pPos.x + 600.f, 540.f);
        window.setView(camera);
        float vL = camera.getCenter().x - 960.f;
        float vR = camera.getCenter().x + 960.f;

        
        sf::RectangleShape rect(sf::Vector2f(vR - vL, 150.f));
        rect.setPosition(vL, 0.f); rect.setFillColor(sf::Color(0, 105, 148)); window.draw(rect);
        rect.setPosition(vL, 930.f); rect.setFillColor(sf::Color(34, 139, 34)); window.draw(rect);
        rect.setSize(sf::Vector2f(vR - vL, 720.f)); rect.setPosition(vL, 180.f); rect.setFillColor(sf::Color(45, 45, 45)); window.draw(rect);
        rect.setSize(sf::Vector2f(vR - vL, 30.f)); rect.setFillColor(sf::Color(130, 130, 130)); rect.setPosition(vL, 150.f); window.draw(rect);
        rect.setPosition(vL, 900.f); window.draw(rect); rect.setSize(sf::Vector2f(vR - vL, 10.f)); rect.setPosition(vL, 540.f); rect.setFillColor(sf::Color(20, 20, 20)); window.draw(rect);
        sf::RectangleShape line(sf::Vector2f(50.f, 5.f)); line.setFillColor(sf::Color(200, 200, 200, 150));

        for (int i = (int)(vL / 150.f) - 1; i < (int)(vR / 150.f) + 1; ++i) {
            for (float ly : {300.f, 420.f, 670.f, 790.f}) { 
                line.setPosition(i * 150.f, ly); 
                window.draw(line); 
            }
        }

        for (auto& npc : traffic) {
            Vec2 rPos = Lerp(npc.previousPosition, npc.position, alpha);
            npc.shape.setPosition(rPos.x, rPos.y); window.draw(npc.shape);
        }
        player.shape.setPosition(pPos.x, pPos.y); window.draw(player.shape);
        window.display();
    }
    return 0;
}