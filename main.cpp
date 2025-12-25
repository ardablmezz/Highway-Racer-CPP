#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

enum GameState {
	PLAYING,
	GAMEOVER
};

struct Vec2 {
    float x, y;
};

Vec2 Lerp(const Vec2& a, const Vec2& b, float alpha) {
    return { a.x * (1.0f - alpha) + b.x * alpha, a.y * (1.0f - alpha) + b.y * alpha };
}

struct Prop {
    sf::Sprite sprite;
    float x, y;
};

struct Car {
    int currentLane;
    float targetY;
    bool isChangingLane;
    bool canOvertake;
    Vec2 position;
    Vec2 previousPosition;
    Vec2 velocity;
    Vec2 acceleration;
    float maxSpeed;
    float damping;
    float laneChangeCooldown;
    sf::Sprite sprite;

    Car(Vec2 pos, Vec2 vel, sf::Texture& tex, sf::IntRect imgRect, float maxSpeed_, float damping_, int startLane) {
        position = pos;
        previousPosition = pos;
        velocity = vel;
        acceleration = { 0.f, 0.f };
        maxSpeed = maxSpeed_;
        damping = damping_;
        laneChangeCooldown = 0.f;
        sprite.setTexture(tex);
        sprite.setTextureRect(imgRect);
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        sprite.setScale(1.1f, 1.1f);
        isChangingLane = false;
        currentLane = startLane;
        targetY = pos.y;
    }

    void update(float dt, bool isPlayer = false) {
        previousPosition = position;

        if (laneChangeCooldown > 0.f) {
            laneChangeCooldown -= dt;
        }

        velocity.x += acceleration.x * dt;
        velocity.y += acceleration.y * dt;

        if (acceleration.x == 0.f) velocity.x *= damping;
        if (acceleration.y == 0.f) velocity.y *= damping;

        float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (speed > maxSpeed) {
            velocity.x = (velocity.x / speed) * maxSpeed;
            velocity.y = (velocity.y / speed) * maxSpeed;
        }

        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        sprite.setPosition(position.x, position.y);

        if (isPlayer) {
            sprite.setRotation(90.f);
        }
        else {
            if (currentLane <= 2) { sprite.setRotation(270.f); }
            else { sprite.setRotation(90.f); }
        }
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Highway Racer - v3.1");
    window.setFramerateLimit(144);
    sf::View camera(sf::FloatRect(0.f, .0f, 1920.f, 1080.f));
    std::random_device rd;
    std::mt19937 gen(rd());

	GameState currentState = PLAYING;
    sf::SoundBuffer engineBuffer;
    bool engineLoaded = engineBuffer.loadFromFile("assets/engine.wav");

    sf::Sound engineSound;
    if (engineLoaded) {
        engineSound.setBuffer(engineBuffer);
        engineSound.setLoop(true);
        engineSound.setVolume(85.f);
        engineSound.play();
    }
    else {
        std::cout << "HATA: engine.wav bulunamadi." << std::endl;
    }

    sf::SoundBuffer crashBuffer;
    bool crashLoaded = crashBuffer.loadFromFile("assets/crash.wav");

    sf::Sound crashSound;
    if (crashLoaded) {
        crashSound.setBuffer(crashBuffer);
        crashSound.setVolume(80.f);
    }
    
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        if (!font.loadFromFile("assets/fixedsys.fon")) {
            std::cout << "HATA: Font dosyasi yuklenemedi!" << std::endl;
        }
    }
    sf::Text gameOverText;
    gameOverText.setFont(font);
    gameOverText.setString("GAME OVER");
    gameOverText.setCharacterSize(100);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setStyle(sf::Text::Bold);
    gameOverText.setOutlineColor(sf::Color::Black);
    gameOverText.setOutlineThickness(5.f);

    sf::FloatRect goBounds = gameOverText.getLocalBounds();
    gameOverText.setOrigin(goBounds.width / 2.f, goBounds.height / 2.f);

    sf::Text restartText;
    restartText.setFont(font);
    restartText.setString("PRESS 'R' TO RESTART");
    restartText.setCharacterSize(40);
    restartText.setFillColor(sf::Color::White);
    restartText.setOutlineColor(sf::Color::Black);
    restartText.setOutlineThickness(3.f);

    sf::FloatRect rBounds = restartText.getLocalBounds();
    restartText.setOrigin(rBounds.width / 2.f, rBounds.height / 2.f);

    sf::Texture carsTexture;
    if (!carsTexture.loadFromFile("assets/bk_cars1.a.png")) return -1;

    sf::Texture treeTexture;
    bool treeLoaded = treeTexture.loadFromFile("assets/tree1.png");

    sf::Texture waterTexture;
    bool waterLoaded = waterTexture.loadFromFile("assets/water.jpg");
    if (waterLoaded) {
        waterTexture.setRepeated(true);
    }
    sf::Sprite waterSprite;
    if (waterLoaded) {
        waterSprite.setTexture(waterTexture);
        waterSprite.setScale(1.f, 1.f);
    }
    float waterScrollSpeed = 80.f;
    float currentWaterOffset = 0.f;

    sf::IntRect playerRect(72, 446, 51, 96);
    std::vector<sf::IntRect> npcTypes = {
        sf::IntRect(7, 16, 52, 135),
        sf::IntRect(129, 16, 49, 93),
        sf::IntRect(189, 16, 50, 97),
        sf::IntRect(426, 21, 60, 137),
        sf::IntRect(308, 20, 52, 92),
        sf::IntRect(247, 123, 52, 92),
        sf::IntRect(189, 120, 51, 100),
        sf::IntRect(65, 120, 51, 100),
        sf::IntRect(186, 226, 55, 103),
        sf::IntRect(308, 223, 55, 106),
        sf::IntRect(236, 346, 45, 86),
        sf::IntRect(10, 346, 45, 86)
    };

    std::vector<sf::IntRect> treeTypes = {
        sf::IntRect(395, 5, 108, 122),
        sf::IntRect(258, 260, 116, 124),
        sf::IntRect(260, 385, 122, 126)
    };
    std::uniform_real_distribution<float> speedDist(250.f, 500.f);

    float lanesY[6] = { 240.f, 360.f, 480.f, 610.f, 730.f, 845.f };
    const float fixedDt = 1.0f / 60.0f;
    float accumulator = 0.0f;
    sf::Clock clock;

    Car player({ 300.f, 810.f }, { 0.f, 0.f }, carsTexture, playerRect, 600.f, 0.96f, 5);
    float playerAccelX = 600.f;
    float playerAccelY = 500.f;

    std::vector<Car> traffic;

    for (int i = 0; i < 34; ++i) {
        int laneIndex;
        float dirX;
        if (i < 15) {
            laneIndex = std::uniform_int_distribution<>(0, 2)(gen);
            dirX = -speedDist(gen);
        }
        else {
            laneIndex = std::uniform_int_distribution<>(3, 5)(gen);
            dirX = speedDist(gen);
        }

        bool willOvertake = (std::uniform_int_distribution<>(1, 3)(gen) == 1);
        float spawnX = static_cast<float>((gen() % 15000) - 5000);

        int chance = std::uniform_int_distribution<>(1, 100)(gen);
        int selectedIdx = 0;
        if (chance <= 3) { selectedIdx = 0; }
        else if (chance <= 7) { selectedIdx = 1; }
        else if (chance <= 10) { selectedIdx = 2; }
        else if (chance <= 13) { selectedIdx = 3; }
        else {
            selectedIdx = std::uniform_int_distribution<>(4, (int)npcTypes.size() - 1)(gen);
        }

        float npcSpeed = std::uniform_real_distribution<float>(350.f, 550.f)(gen);
        traffic.emplace_back(Vec2{ spawnX, lanesY[laneIndex] }, Vec2{ dirX, 0.f }, carsTexture, npcTypes[selectedIdx], npcSpeed, 1.0f, laneIndex);
        traffic.back().canOvertake = willOvertake;
    }

    std::vector<Prop> forest;
    float forestChunkSize = 25000.f;

    if (treeLoaded && !treeTypes.empty()) {
        for (float y = 1000.f; y < 1100.f; y += 30.f) {
            for (float x = -5000.f; x < 20000.f; x += 130.f) {
                Prop p;
                p.sprite.setTexture(treeTexture);
                int typeIdx = std::uniform_int_distribution<>(0, (int)treeTypes.size() - 1)(gen);
                p.sprite.setTextureRect(treeTypes[typeIdx]);
                float offsetX = (float)(gen() % 100);
                float offsetY = (float)(gen() % 40);
                p.x = x + offsetX;
                p.y = y + offsetY;
                p.sprite.setPosition(p.x, p.y);
                sf::FloatRect b = p.sprite.getLocalBounds();
                p.sprite.setOrigin(b.width / 2.f, b.height);
                float depthScale = 1.0f + ((y - 920.f) / 230.f) * 0.5f;
                p.sprite.setScale(depthScale, depthScale);

                forest.push_back(p);
            }
        }
        std::sort(forest.begin(), forest.end(), [](const Prop& a, const Prop& b) {
            return a.y < b.y;
            });
    }

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) 
                window.close();
            if (currentState == GAMEOVER && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                currentState = PLAYING;
				int restartLane = std::uniform_int_distribution<>(3, 5)(gen);
                player.position = { 300.f, lanesY[restartLane]};
                player.velocity = { 0.f, 0.f };
                player.acceleration = { 0.f, 0.f };
                traffic.clear();
                for (int i = 0; i < 34; ++i) {
                    int laneIndex = (i < 15) ? std::uniform_int_distribution<>(0, 2)(gen) : std::uniform_int_distribution<>(3, 5)(gen);
                    float dirX = (i < 15) ? -speedDist(gen) : speedDist(gen);
                    float spawnX = static_cast<float>((gen() % 15000) - 5000);
                    int chance = std::uniform_int_distribution<>(1, 100)(gen);
                    int selectedIdx = 0;
                    if (chance <= 3) selectedIdx = 0;
                    else if (chance <= 7) selectedIdx = 1;
                    else if (chance <= 10) selectedIdx = 2;
                    else if (chance <= 13) selectedIdx = 3;
                    else selectedIdx = std::uniform_int_distribution<>(4, (int)npcTypes.size() - 1)(gen);
                    float npcSpeed = std::uniform_real_distribution<float>(350.f, 550.f)(gen);
                    traffic.emplace_back(Vec2{ spawnX, lanesY[laneIndex] }, Vec2{ dirX, 0.f }, carsTexture, npcTypes[selectedIdx], npcSpeed, 1.0f, laneIndex);
                    traffic.back().canOvertake = (std::uniform_int_distribution<>(1, 3)(gen) == 1);
                }
                if (engineLoaded) {
                    engineSound.setPitch(1.0f);
                    if (engineSound.getStatus() != sf::Sound::Playing)
                        engineSound.play();
                }
            }
        }

        float frameTime = clock.restart().asSeconds();
        if (frameTime > 0.25f) frameTime = 0.25f;
        if (currentState == PLAYING) {
			accumulator += frameTime;
		}
        else {
            accumulator = 0;
        }

        if (waterLoaded && currentState==PLAYING) {
            currentWaterOffset += waterScrollSpeed * frameTime;
        }

        float speedFactor = std::abs(player.velocity.x) / player.maxSpeed;
        float maneuverability = 1.0f - (speedFactor * 0.32f);

        player.acceleration = { 0.f, 0.f };
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) player.acceleration.y = -playerAccelY * maneuverability;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) player.acceleration.y = playerAccelY * maneuverability;
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
                            bool overtaked = false;
                            if (traffic[i].canOvertake && !traffic[i].isChangingLane && traffic[i].laneChangeCooldown <= 0.f) {
                                int side = (traffic[i].currentLane <= 2) ? 0 : 1;
                                int minL = (side == 0) ? 0 : 3;
                                int maxL = (side == 0) ? 2 : 5;
                                bool upFree = (traffic[i].currentLane > minL);
                                bool downFree = (traffic[i].currentLane < maxL);

                                if (traffic[i].currentLane == 2) downFree = false;
                                if (traffic[i].currentLane == 3) upFree = false;

                                float safeZoneNPC = 310.f;
                                float safeZonePlayer = 370.f;

                                for (size_t k = 0; k < traffic.size(); ++k) {
                                    if (i == k) continue;
                                    if (upFree && traffic[k].currentLane == traffic[i].currentLane - 1) {
                                        if (std::abs(traffic[k].position.x - traffic[i].position.x) < safeZoneNPC) upFree = false;
                                    }
                                    if (downFree && traffic[k].currentLane == traffic[i].currentLane + 1) {
                                        if (std::abs(traffic[k].position.x - traffic[i].position.x) < safeZoneNPC) downFree = false;
                                    }
                                }

                                float pDist = player.position.x - traffic[i].position.x;
                                if (upFree && std::abs(player.position.y - lanesY[traffic[i].currentLane - 1]) < 40.f) {
                                    if (std::abs(pDist) < safeZonePlayer) upFree = false;
                                }
                                if (downFree && std::abs(player.position.y - lanesY[traffic[i].currentLane + 1]) < 40.f) {
                                    if (std::abs(pDist) < safeZonePlayer) downFree = false;
                                }

                                if (upFree) {
                                    traffic[i].targetY = lanesY[traffic[i].currentLane - 1];
                                    traffic[i].currentLane--;
                                    traffic[i].isChangingLane = true;
                                    traffic[i].laneChangeCooldown = 2.0f;
                                    overtaked = true;
                                }
                                else if (downFree) {
                                    traffic[i].targetY = lanesY[traffic[i].currentLane + 1];
                                    traffic[i].currentLane++;
                                    traffic[i].isChangingLane = true;
                                    traffic[i].laneChangeCooldown = 2.0f;
                                    overtaked = true;
                                }
                            }

                            if (!overtaked) {
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
                }

                if (traffic[i].isChangingLane) {
                    float yDiff = traffic[i].targetY - traffic[i].position.y;
                    if (std::abs(yDiff) > 2.0f) {
                        traffic[i].velocity.y = (yDiff > 0 ? 195.f : -195.f);
                    }
                    else {
                        traffic[i].position.y = traffic[i].targetY;
                        traffic[i].velocity.y = 0.f;
                        traffic[i].isChangingLane = false;
                    }
                }
                else {
                    traffic[i].velocity.y = 0.f;
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

                if (brake) traffic[i].velocity.x *= 0.95f;
                else if (gas || currentAbs < 450.f) {
                    float acc = (traffic[i].velocity.x >= 0) ? 220.f : -220.f;
                    traffic[i].velocity.x += acc * fixedDt;
                }

                traffic[i].update(fixedDt);

                if (traffic[i].currentLane == 2 && traffic[i].position.y > lanesY[2] + 2.f) {
                    traffic[i].position.y = lanesY[2];
                    traffic[i].velocity.y = 0.f;
                    traffic[i].isChangingLane = false;
                    traffic[i].targetY = lanesY[2];
                }
                if (traffic[i].currentLane == 3 && traffic[i].position.y < lanesY[3] - 2.f) {
                    traffic[i].position.y = lanesY[3];
                    traffic[i].velocity.y = 0.f;
                    traffic[i].isChangingLane = false;
                    traffic[i].targetY = lanesY[3];
                }

                if (traffic[i].position.x < vL - 1500.f || traffic[i].position.x > vR + 3500.f) {
                    bool movingRight = (traffic[i].velocity.x > 0);
                    float randomOffset = (float)(gen() % 2000);
                    float newX = movingRight ? (vL - 1000.f - randomOffset) : (vR + 3000.f + randomOffset);
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
                        traffic[i].currentLane = newLane;
                        traffic[i].targetY = lanesY[newLane];
                        traffic[i].isChangingLane = false;
                        traffic[i].previousPosition = traffic[i].position;
                    }
                }
            }

            player.update(fixedDt, true);

            if (engineLoaded) {
                float speedRatio = std::abs(player.velocity.x) / player.maxSpeed;
                float newPitch = 1.1f + (speedRatio * 1.2f);
                engineSound.setPitch(newPitch);
            }

            for (auto& npc : traffic) {
                sf::FloatRect pB = player.sprite.getGlobalBounds();
                sf::FloatRect nB = npc.sprite.getGlobalBounds();
                sf::FloatRect overlap;
                if (pB.intersects(nB, overlap)) {

                    if (std::abs(player.velocity.x) >= player.maxSpeed * 0.85f) {
                        if (currentState == PLAYING) {
                            currentState = GAMEOVER;
                            engineSound.stop();
                            crashSound.play();
                        }
                        break;
                    }
                    else {
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
            }
            accumulator -= fixedDt;
            float forestChunk = 20000.f;
            for (auto& p : forest) {
                if (p.x < camera.getCenter().x - 3000.f) {
                    p.x += forestChunk;
                    p.sprite.setPosition(p.x, p.y);
                }
                else if (p.x > camera.getCenter().x + 17000.f) {
                    p.x -= forestChunk;
                    p.sprite.setPosition(p.x, p.y);
                }
            }
        }

        float alpha = accumulator / fixedDt;
        window.clear(sf::Color(30, 30, 30));
		Vec2 pPos = (currentState == PLAYING) ? Lerp(player.previousPosition, player.position, alpha) : player.position;
        camera.setCenter(pPos.x + 600.f, 540.f);
        window.setView(camera);
        float vL = camera.getCenter().x - 960.f;
        float vR = camera.getCenter().x + 960.f;

        if (waterLoaded) {
            int texX = (int)(currentWaterOffset + vL);
            int texY = (int)(currentWaterOffset * 0.2f);
            waterSprite.setTextureRect(sf::IntRect(texX, texY, (int)((vR - vL) / waterSprite.getScale().x), (int)(150 / waterSprite.getScale().y)));
            waterSprite.setPosition(vL, 0.f);
            window.draw(waterSprite);
        }
        else {
            sf::RectangleShape seaRect(sf::Vector2f(vR - vL, 150.f));
            seaRect.setPosition(vL, 0.f);
            seaRect.setFillColor(sf::Color(0, 105, 148));
            window.draw(seaRect);
        }

        sf::RectangleShape rect(sf::Vector2f(vR - vL, 150.f));
        rect.setPosition(vL, 930.f); rect.setFillColor(sf::Color(97, 59, 5)); window.draw(rect);
        rect.setSize(sf::Vector2f(vR - vL, 720.f)); rect.setPosition(vL, 180.f); rect.setFillColor(sf::Color(45, 45, 45)); window.draw(rect);
        rect.setSize(sf::Vector2f(vR - vL, 30.f)); rect.setFillColor(sf::Color(130, 130, 130)); rect.setPosition(vL, 150.f); window.draw(rect);
        rect.setPosition(vL, 900.f); window.draw(rect); rect.setSize(sf::Vector2f(vR - vL, 10.f)); rect.setPosition(vL, 540.f); rect.setFillColor(sf::Color(180, 150, 20)); window.draw(rect);
        sf::RectangleShape line(sf::Vector2f(50.f, 5.f)); line.setFillColor(sf::Color(200, 200, 200, 150));

        for (int i = (int)(vL / 150.f) - 1; i < (int)(vR / 150.f) + 1; ++i) {
            for (float ly : {300.f, 420.f, 670.f, 790.f}) {
                line.setPosition(i * 150.f, ly);
                window.draw(line);
            }
        }
        if (treeLoaded) {
            for (const auto& p : forest) {
                if (p.x > vL - 200.f && p.x < vR + 200.f) {
                    window.draw(p.sprite);
                }
            }
        }

        for (auto& npc : traffic) {
			Vec2 rPos = (currentState == PLAYING) ? Lerp(npc.previousPosition, npc.position, alpha) : npc.position;
            npc.sprite.setPosition(rPos.x, rPos.y); window.draw(npc.sprite);
        }
        player.sprite.setPosition(pPos.x, pPos.y); 
        window.draw(player.sprite);
		if (currentState == GAMEOVER) {
			sf::Vector2f center = camera.getCenter();
			sf::RectangleShape overlay(sf::Vector2f(1920.f, 1080.f));
			overlay.setOrigin(960.f, 540.f);
			overlay.setPosition(center);
			overlay.setFillColor(sf::Color(0, 0, 0, 150));
			window.draw(overlay);

			gameOverText.setPosition(center.x, center.y - 50.f);
			restartText.setPosition(center.x, center.y + 70.f);

			window.draw(gameOverText);
			window.draw(restartText);
            }
        window.display();
    }
    return 0;
}