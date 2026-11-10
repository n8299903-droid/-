import math
import random
import pygame

WIDTH, HEIGHT = 1100, 720
WORLD = pygame.Rect(30, 30, 760, 650)
PANEL = pygame.Rect(810, 30, 260, 650)
BG = (15, 19, 27)
GRID = (31, 40, 53)
WHITE = (225, 233, 245)
MUTED = (135, 151, 173)
GREEN = (78, 230, 145)
CYAN = (76, 205, 255)
RED = (255, 100, 115)
YELLOW = (255, 205, 90)


class Target:
    def __init__(self):
        self.respawn()

    def respawn(self):
        self.x = random.uniform(WORLD.left + 50, WORLD.right - 50)
        self.y = random.uniform(WORLD.top + 50, WORLD.bottom - 50)
        angle = random.random() * math.tau
        speed = random.uniform(35, 95)
        self.vx, self.vy = math.cos(angle) * speed, math.sin(angle) * speed
        self.name = random.choice(["BOT_ALPHA", "BOT_BRAVO", "BOT_CHARLIE", "BOT_DELTA"])
        self.distance = random.uniform(8, 65)

    def update(self, dt):
        self.x += self.vx * dt
        self.y += self.vy * dt
        if self.x < WORLD.left + 20 or self.x > WORLD.right - 20:
            self.vx *= -1
        if self.y < WORLD.top + 20 or self.y > WORLD.bottom - 20:
            self.vy *= -1
        self.x = max(WORLD.left + 20, min(WORLD.right - 20, self.x))
        self.y = max(WORLD.top + 20, min(WORLD.bottom - 20, self.y))


class App:
    def __init__(self):
        pygame.init()
        pygame.display.set_caption("VisualHUD — Training Simulator")
        self.screen = pygame.display.set_mode((WIDTH, HEIGHT))
        self.clock = pygame.time.Clock()
        self.font = pygame.font.SysFont("consolas", 16)
        self.small = pygame.font.SysFont("consolas", 12)
        self.big = pygame.font.SysFont("consolas", 22, bold=True)
        self.targets = [Target() for _ in range(8)]
        self.esp = self.radar = self.names = True
        self.paused = False
        self.count = 8
        self.buttons = {}
        self.running = True

    def txt(self, text, x, y, color=WHITE, font=None):
        self.screen.blit((font or self.font).render(text, True, color), (x, y))

    def toggle(self, key, label, y):
        rect = pygame.Rect(PANEL.x + 18, y, PANEL.width - 36, 34)
        self.buttons[key] = rect
        pygame.draw.rect(self.screen, (31, 40, 54), rect, border_radius=6)
        state = getattr(self, key)
        pygame.draw.circle(self.screen, GREEN if state else MUTED, (rect.x + 18, rect.centery), 6)
        self.txt(label, rect.x + 34, rect.y + 8)
        self.txt("ON" if state else "OFF", rect.right - 48, rect.y + 8,
                 GREEN if state else MUTED, self.small)

    def draw_world(self):
        pygame.draw.rect(self.screen, (19, 25, 35), WORLD, border_radius=8)
        for x in range(WORLD.left, WORLD.right, 40):
            pygame.draw.line(self.screen, GRID, (x, WORLD.top), (x, WORLD.bottom))
        for y in range(WORLD.top, WORLD.bottom, 40):
            pygame.draw.line(self.screen, GRID, (WORLD.left, y), (WORLD.right, y))
        pygame.draw.rect(self.screen, (67, 82, 105), WORLD, 2, border_radius=8)
        px, py = WORLD.center
        pygame.draw.circle(self.screen, CYAN, (px, py), 9, 2)
        pygame.draw.line(self.screen, CYAN, (px - 14, py), (px + 14, py), 1)
        pygame.draw.line(self.screen, CYAN, (px, py - 14), (px, py + 14), 1)
        self.txt("TRAINING ARENA / SYNTHETIC ENTITIES ONLY",
                 WORLD.x + 14, WORLD.y + 12, MUTED, self.small)

        for target in self.targets:
            x, y = int(target.x), int(target.y)
            pygame.draw.circle(self.screen, RED, (x, y), 8)
            pygame.draw.circle(self.screen, (255, 190, 195), (x, y), 3)
            if self.esp:
                box = pygame.Rect(x - 17, y - 27, 34, 54)
                pygame.draw.rect(self.screen, GREEN, box, 2, border_radius=3)
                pygame.draw.line(self.screen, GREEN, (x, y - 34), (x, y - 27), 2)
                if self.names:
                    label = f"{target.name}  {target.distance:.0f}m"
                    surface = self.small.render(label, True, GREEN)
                    self.screen.blit(surface, (x - surface.get_width() // 2, y - 45))

    def draw_radar(self):
        if not self.radar:
            return
        radar = pygame.Rect(PANEL.x + 28, 405, 204, 204)
        pygame.draw.rect(self.screen, (12, 17, 25), radar, border_radius=8)
        pygame.draw.rect(self.screen, (65, 82, 105), radar, 1, border_radius=8)
        pygame.draw.line(self.screen, GRID, (radar.centerx, radar.top + 8),
                         (radar.centerx, radar.bottom - 8))
        pygame.draw.line(self.screen, GRID, (radar.left + 8, radar.centery),
                         (radar.right - 8, radar.centery))
        pygame.draw.circle(self.screen, CYAN, radar.center, 5)
        scale = 0.12
        for target in self.targets:
            dx = (target.x - WORLD.centerx) * scale
            dy = (target.y - WORLD.centery) * scale
            px = max(radar.left + 8, min(radar.right - 8, int(radar.centerx + dx)))
            py = max(radar.top + 8, min(radar.bottom - 8, int(radar.centery + dy)))
            pygame.draw.circle(self.screen, RED, (px, py), 4)
        self.txt("RADAR", radar.x, radar.y - 23, MUTED, self.small)

    def draw_panel(self):
        pygame.draw.rect(self.screen, (22, 29, 40), PANEL, border_radius=10)
        self.txt("VISUALHUD", PANEL.x + 18, PANEL.y + 18, CYAN, self.big)
        self.txt("TRAINING BUILD  •  v1.0", PANEL.x + 18, PANEL.y + 50, MUTED, self.small)
        pygame.draw.line(self.screen, GRID, (PANEL.x + 16, 100), (PANEL.right - 16, 100))
        self.txt("OVERLAY MODULES", PANEL.x + 18, 116, MUTED, self.small)
        self.toggle("esp", "ESP BOXES", 140)
        self.toggle("radar", "MINI RADAR", 182)
        self.toggle("names", "NAMES + DISTANCE", 224)
        self.txt("SIMULATION", PANEL.x + 18, 282, MUTED, self.small)
        self.txt(f"Targets: {self.count}   [-]   [+]", PANEL.x + 18, 310)
        self.buttons["minus"] = pygame.Rect(PANEL.x + 18, 338, 48, 30)
        self.buttons["plus"] = pygame.Rect(PANEL.x + 74, 338, 48, 30)
        self.buttons["pause"] = pygame.Rect(PANEL.x + 130, 338, 102, 30)
        for key, label in [("minus", "-"), ("plus", "+"),
                           ("pause", "RESUME" if self.paused else "PAUSE")]:
            rect = self.buttons[key]
            pygame.draw.rect(self.screen, (42, 55, 73), rect, border_radius=5)
            surface = self.small.render(label, True, WHITE)
            self.screen.blit(surface, surface.get_rect(center=rect.center))
        self.draw_radar()
        self.txt("Legend", PANEL.x + 18, 630, MUTED, self.small)
        pygame.draw.circle(self.screen, RED, (PANEL.x + 28, 654), 5)
        self.txt("Training target", PANEL.x + 42, 648, WHITE, self.small)
        self.txt("No CS2 / VAC integration", PANEL.x + 18, 682, YELLOW, self.small)

    def click(self, position):
        for key, rect in self.buttons.items():
            if not rect.collidepoint(position):
                continue
            if key in ("esp", "radar", "names"):
                setattr(self, key, not getattr(self, key))
            elif key == "minus":
                self.count = max(1, self.count - 1)
            elif key == "plus":
                self.count = min(30, self.count + 1)
            elif key == "pause":
                self.paused = not self.paused
            self.targets = self.targets[:self.count]
            while len(self.targets) < self.count:
                self.targets.append(Target())
            return

    def run(self):
        while self.running:
            dt = min(self.clock.tick(60) / 1000, 0.05)
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False
                elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                    self.click(event.pos)
                elif event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                    self.running = False
            if not self.paused:
                for target in self.targets:
                    target.update(dt)
            self.screen.fill(BG)
            self.draw_world()
            self.draw_panel()
            pygame.display.flip()
        pygame.quit()


if __name__ == "__main__":
    App().run()
