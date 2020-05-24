import pyglet
from pyglet.window import key

from mumble_link import MumbleConnection, Point

game_window = pyglet.window.Window(800, 600)

pyglet.resource.path = ["resources"]
pyglet.resource.reindex()

avatar_image = pyglet.resource.image("avatar.png")


def center_image(image):
    """Sets an image's anchor point to its center"""
    image.anchor_x = image.width // 2
    image.anchor_y = image.height // 2


center_image(avatar_image)


class Player(pyglet.sprite.Sprite):
    def __init__(self, *args, **kwargs):
        super().__init__(img=avatar_image, *args, **kwargs)
        self.key_handler = key.KeyStateHandler()
        self.speed = 10

    def update(self, dt):
        if self.key_handler[key.LEFT]:
            self.x -= self.speed
        if self.key_handler[key.RIGHT]:
            self.x += self.speed
        if self.key_handler[key.UP]:
            self.y += self.speed
        if self.key_handler[key.DOWN]:
            self.y -= self.speed


me = Player(x=game_window.width // 2, y=game_window.height // 2)


def update(dt):
    me.update(dt)


@game_window.event
def on_draw():
    game_window.clear()
    me.draw()


mumble_params = dict(
    app_name="TestLink",
    app_description="TestLink is a test of the Link plugin.",
    context=b"test-context",
    user="tora-dp",
)


with MumbleConnection(**mumble_params) as mumble:

    def sync_mumble(dt):
        mumble.position = Point(me.x, me.y)
        mumble.set_rotation(90)
        mumble.tick()
        mumble.sync()

    pyglet.clock.schedule_interval(update, 1 / 120.0)
    pyglet.clock.schedule_interval(sync_mumble, 1 / 10.0)
    game_window.push_handlers(me.key_handler)

    pyglet.app.run()
