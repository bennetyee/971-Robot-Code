import cairo
from color import Color, palette
from constants import *
import numpy as np


def set_color(cr, color, a=1):
    if color.a == 1.0:
        cr.set_source_rgba(color.r, color.g, color.b, a)
    else:
        cr.set_source_rgba(color.r, color.g, color.b, color.a)


def draw_px_cross(cr, x, y, length_px, color=palette["RED"]):
    """Draws a cross with fixed dimensions in pixel space."""
    set_color(cr, color)
    cr.move_to(x, y - length_px)
    cr.line_to(x, y + length_px)
    cr.stroke()

    cr.move_to(x - length_px, y)
    cr.line_to(x + length_px, y)
    cr.stroke()
    set_color(cr, palette["WHITE"])


def draw_px_x(cr, x, y, length_px1, color=palette["BLACK"]):
    """Draws a x with fixed dimensions in pixel space."""
    length_px = length_px1 / np.sqrt(2)
    set_color(cr, color)
    cr.move_to(x - length_px, y - length_px)
    cr.line_to(x + length_px, y + length_px)
    cr.stroke()

    cr.move_to(x - length_px, y + length_px)
    cr.line_to(x + length_px, y - length_px)
    cr.stroke()
    set_color(cr, palette["WHITE"])


def draw_control_points(cr, points, width=10, radius=4, color=palette["BLUE"]):
    for i in range(0, len(points)):
        draw_px_x(cr, points[i][0], points[i][1], width, color)
        set_color(cr, color)
        cr.arc(points[i][0], points[i][1], radius, 0, 2.0 * np.pi)
        cr.fill()
        set_color(cr, palette["WHITE"])


def display_text(cr, text, widtha, heighta, widthb, heightb):
    cr.scale(widtha, -heighta)
    cr.show_text(text)
    cr.scale(widthb, -heightb)


def draw_HAB(cr):
    # BASE Constants
    X_BASE = 0
    Y_BASE = 0
    R = 0.381 - .1
    BACKWALL_X = X_BASE
    LOADING_Y = mToPx(4.129151) - mToPx(0.696976)
    # HAB Levels 2 and 3 called in variables backhab
    # draw loading stations
    cr.move_to(0, LOADING_Y)
    cr.line_to(mToPx(0.6), LOADING_Y)
    cr.move_to(mToPx(R), LOADING_Y)
    cr.arc(mToPx(R), LOADING_Y, 5, 0, np.pi * 2.0)
    cr.move_to(0, -1.0 * LOADING_Y)
    cr.line_to(mToPx(0.6), -1.0 * LOADING_Y)
    cr.move_to(mToPx(R), -1.0 * LOADING_Y)
    cr.arc(mToPx(R), -1.0 * LOADING_Y, 5, 0, np.pi * 2.0)

    # HAB Levels 2 and 3 called in variables backhab
    WIDTH_BACKHAB = mToPx(1.2192)

    Y_TOP_BACKHAB_BOX = Y_BASE + mToPx(0.6096)
    BACKHAB_LV2_LENGTH = mToPx(1.016)

    BACKHAB_LV3_LENGTH = mToPx(1.2192)
    Y_LV3_BOX = Y_TOP_BACKHAB_BOX - BACKHAB_LV3_LENGTH

    Y_BOTTOM_BACKHAB_BOX = Y_LV3_BOX - BACKHAB_LV2_LENGTH

    # HAB LEVEL 1
    X_LV1_BOX = BACKWALL_X + WIDTH_BACKHAB

    WIDTH_LV1_BOX = mToPx(0.90805)
    LENGTH_LV1_BOX = mToPx(1.6256)

    Y_BOTTOM_LV1_BOX = Y_BASE - LENGTH_LV1_BOX

    # Ramp off Level 1
    X_RAMP = X_LV1_BOX

    Y_TOP_RAMP = Y_BASE + LENGTH_LV1_BOX
    WIDTH_TOP_RAMP = mToPx(1.20015)
    LENGTH_TOP_RAMP = Y_BASE + mToPx(0.28306)

    X_MIDDLE_RAMP = X_RAMP + WIDTH_LV1_BOX
    Y_MIDDLE_RAMP = Y_BOTTOM_LV1_BOX
    LENGTH_MIDDLE_RAMP = 2 * LENGTH_LV1_BOX
    WIDTH_MIDDLE_RAMP = WIDTH_TOP_RAMP - WIDTH_LV1_BOX

    Y_BOTTOM_RAMP = Y_BASE - LENGTH_LV1_BOX - LENGTH_TOP_RAMP

    # Side Bars to Hold in balls
    X_BARS = BACKWALL_X
    WIDTH_BARS = WIDTH_BACKHAB
    LENGTH_BARS = mToPx(0.574675)

    Y_TOP_BAR = Y_TOP_BACKHAB_BOX + BACKHAB_LV2_LENGTH

    Y_BOTTOM_BAR = Y_BOTTOM_BACKHAB_BOX - LENGTH_BARS

    set_color(cr, palette["BLACK"])
    cr.rectangle(BACKWALL_X, Y_TOP_BACKHAB_BOX, WIDTH_BACKHAB,
                 BACKHAB_LV2_LENGTH)
    cr.rectangle(BACKWALL_X, Y_LV3_BOX, WIDTH_BACKHAB, BACKHAB_LV3_LENGTH)
    cr.rectangle(BACKWALL_X, Y_BOTTOM_BACKHAB_BOX, WIDTH_BACKHAB,
                 BACKHAB_LV2_LENGTH)
    cr.rectangle(X_LV1_BOX, Y_BASE, WIDTH_LV1_BOX, LENGTH_LV1_BOX)
    cr.rectangle(X_LV1_BOX, Y_BOTTOM_LV1_BOX, WIDTH_LV1_BOX, LENGTH_LV1_BOX)
    cr.rectangle(X_RAMP, Y_TOP_RAMP, WIDTH_TOP_RAMP, LENGTH_TOP_RAMP)
    cr.rectangle(X_MIDDLE_RAMP, Y_MIDDLE_RAMP, WIDTH_MIDDLE_RAMP,
                 LENGTH_MIDDLE_RAMP)
    cr.rectangle(X_RAMP, Y_BOTTOM_RAMP, WIDTH_TOP_RAMP, LENGTH_TOP_RAMP)
    cr.rectangle(X_BARS, Y_TOP_BAR, WIDTH_BARS, LENGTH_BARS)
    cr.rectangle(X_BARS, Y_BOTTOM_BAR, WIDTH_BARS, LENGTH_BARS)
    cr.stroke()

    cr.set_line_join(cairo.LINE_JOIN_ROUND)

    cr.stroke()

    #draw 0, 0
    set_color(cr, palette["BLACK"])
    cr.move_to(0, 0)
    cr.line_to(0, 0 + mToPx(8.2296 / 2.0))
    cr.move_to(0, 0)
    cr.line_to(0, 0 + mToPx(-8.2296 / 2.0))
    cr.move_to(0, 0)
    cr.line_to(0 + mToPx(8.2296), 0)

    cr.stroke()


def draw_rockets(cr):
    # BASE Constants
    X_BASE = mToPx(2.41568)
    Y_BASE = 0
    # Robot longitudinal radius
    R = 0.381
    near_side_rocket_center = [
        X_BASE + mToPx((2.89973 + 3.15642) / 2.0), Y_BASE + mToPx(
            (3.86305 + 3.39548) / 2.0)
    ]
    middle_rocket_center = [
        X_BASE + mToPx((3.15642 + 3.6347) / 2.0), Y_BASE + mToPx(
            (3.39548 + 3.392380) / 2.0)
    ]
    far_side_rocket_center = [
        X_BASE + mToPx((3.63473 + 3.89984) / 2.0), Y_BASE + mToPx(
            (3.39238 + 3.86305) / 2.0)
    ]

    cr.move_to(near_side_rocket_center[0], near_side_rocket_center[1])
    cr.line_to(near_side_rocket_center[0] - 0.8 * mToPx(0.866),
               near_side_rocket_center[1] - 0.8 * mToPx(0.5))
    cr.move_to(near_side_rocket_center[0] - R * mToPx(0.866),
               near_side_rocket_center[1] - R * mToPx(0.5))
    cr.arc(near_side_rocket_center[0] - R * mToPx(0.866),
           near_side_rocket_center[1] - R * mToPx(0.5), 5, 0, np.pi * 2.0)

    cr.move_to(middle_rocket_center[0], middle_rocket_center[1])
    cr.line_to(middle_rocket_center[0], middle_rocket_center[1] - mToPx(0.8))
    cr.move_to(middle_rocket_center[0], middle_rocket_center[1] - mToPx(R))
    cr.arc(middle_rocket_center[0], middle_rocket_center[1] - mToPx(R), 5, 0,
           np.pi * 2.0)

    cr.move_to(far_side_rocket_center[0], far_side_rocket_center[1])
    cr.line_to(far_side_rocket_center[0] + 0.8 * mToPx(0.866),
               far_side_rocket_center[1] - 0.8 * mToPx(0.5))
    cr.move_to(far_side_rocket_center[0] + R * mToPx(0.866),
               far_side_rocket_center[1] - R * mToPx(0.5))
    cr.arc(far_side_rocket_center[0] + R * mToPx(0.866),
           far_side_rocket_center[1] - R * mToPx(0.5), 5, 0, np.pi * 2.0)

    #print(far_side_rocket_center)
    near_side_rocket_center = [
        X_BASE + mToPx((2.89973 + 3.15642) / 2.0), Y_BASE - mToPx(
            (3.86305 + 3.39548) / 2.0)
    ]
    middle_rocket_center = [
        X_BASE + mToPx((3.15642 + 3.6347) / 2.0), Y_BASE - mToPx(
            (3.39548 + 3.392380) / 2.0)
    ]
    far_side_rocket_center = [
        X_BASE + mToPx((3.63473 + 3.89984) / 2.0), Y_BASE - mToPx(
            (3.39238 + 3.86305) / 2.0)
    ]

    cr.move_to(near_side_rocket_center[0], near_side_rocket_center[1])
    cr.line_to(near_side_rocket_center[0] - 0.8 * mToPx(0.866),
               near_side_rocket_center[1] + 0.8 * mToPx(0.5))

    cr.move_to(middle_rocket_center[0], middle_rocket_center[1])
    cr.line_to(middle_rocket_center[0], middle_rocket_center[1] + mToPx(0.8))

    cr.move_to(far_side_rocket_center[0], far_side_rocket_center[1])
    cr.line_to(far_side_rocket_center[0] + 0.8 * mToPx(0.866),
               far_side_rocket_center[1] + 0.8 * mToPx(0.5))

    # Leftmost Line
    cr.move_to(X_BASE + mToPx(2.89973), Y_BASE + mToPx(3.86305))
    cr.line_to(X_BASE + mToPx(3.15642), Y_BASE + mToPx(3.39548))

    # Top Line
    cr.move_to(X_BASE + mToPx(3.15642), Y_BASE + mToPx(3.39548))
    cr.line_to(X_BASE + mToPx(3.63473), Y_BASE + mToPx(3.39238))

    #Rightmost Line
    cr.move_to(X_BASE + mToPx(3.63473), Y_BASE + mToPx(3.39238))
    cr.line_to(X_BASE + mToPx(3.89984), Y_BASE + mToPx(3.86305))

    #Back Line
    cr.move_to(X_BASE + mToPx(2.89973), Y_BASE + mToPx(3.86305))
    cr.line_to(X_BASE + mToPx(3.89984), Y_BASE + mToPx(3.86305))

    # Bottom Rocket
    # Leftmost Line
    cr.move_to(X_BASE + mToPx(2.89973), Y_BASE - mToPx(3.86305))
    cr.line_to(X_BASE + mToPx(3.15642), Y_BASE - mToPx(3.39548))

    # Top Line
    cr.move_to(X_BASE + mToPx(3.15642), Y_BASE - mToPx(3.39548))
    cr.line_to(X_BASE + mToPx(3.63473), Y_BASE - mToPx(3.39238))

    #Rightmost Line
    cr.move_to(X_BASE + mToPx(3.63473), Y_BASE - mToPx(3.39238))
    cr.line_to(X_BASE + mToPx(3.89984), Y_BASE - mToPx(3.86305))

    #Back Line
    cr.move_to(X_BASE + mToPx(2.89973), Y_BASE - mToPx(3.86305))
    cr.line_to(X_BASE + mToPx(3.89984), Y_BASE - mToPx(3.86305))

    cr.stroke()


def draw_cargo_ship(cr):
    # BASE Constants
    X_BASE = 0 + mToPx(5.59435)
    Y_BASE = 0 + 0  #mToPx(4.129151)
    R = 0.381 - 0.1

    FRONT_PEG_DELTA_Y = mToPx(0.276352)
    cr.move_to(X_BASE, Y_BASE + FRONT_PEG_DELTA_Y)
    cr.line_to(X_BASE - mToPx(0.8), Y_BASE + FRONT_PEG_DELTA_Y)

    cr.move_to(X_BASE, Y_BASE + FRONT_PEG_DELTA_Y)
    cr.arc(X_BASE - mToPx(R), Y_BASE + FRONT_PEG_DELTA_Y, 5, 0, np.pi * 2.0)

    cr.move_to(X_BASE, Y_BASE - FRONT_PEG_DELTA_Y)
    cr.line_to(X_BASE - mToPx(0.8), Y_BASE - FRONT_PEG_DELTA_Y)

    cr.move_to(X_BASE, Y_BASE - FRONT_PEG_DELTA_Y)
    cr.arc(X_BASE - mToPx(R), Y_BASE - FRONT_PEG_DELTA_Y, 5, 0, np.pi * 2.0)

    SIDE_PEG_Y = mToPx(1.41605 / 2.0)
    SIDE_PEG_X = X_BASE + mToPx(1.148842)
    SIDE_PEG_DX = mToPx(0.55245)

    cr.move_to(SIDE_PEG_X, SIDE_PEG_Y)
    cr.line_to(SIDE_PEG_X, SIDE_PEG_Y + mToPx(0.8))
    cr.move_to(SIDE_PEG_X, SIDE_PEG_Y + mToPx(R))
    cr.arc(SIDE_PEG_X, SIDE_PEG_Y + mToPx(R), 5, 0, np.pi * 2.0)

    cr.move_to(SIDE_PEG_X + SIDE_PEG_DX, SIDE_PEG_Y)
    cr.line_to(SIDE_PEG_X + SIDE_PEG_DX, SIDE_PEG_Y + mToPx(0.8))
    cr.move_to(SIDE_PEG_X + SIDE_PEG_DX, SIDE_PEG_Y + mToPx(R))
    cr.arc(SIDE_PEG_X + SIDE_PEG_DX, SIDE_PEG_Y + mToPx(R), 5, 0, np.pi * 2.0)

    cr.move_to(SIDE_PEG_X + 2.0 * SIDE_PEG_DX, SIDE_PEG_Y)
    cr.line_to(SIDE_PEG_X + 2.0 * SIDE_PEG_DX, SIDE_PEG_Y + mToPx(0.8))
    cr.move_to(SIDE_PEG_X + 2.0 * SIDE_PEG_DX, SIDE_PEG_Y + mToPx(R))
    cr.arc(SIDE_PEG_X + 2.0 * SIDE_PEG_DX, SIDE_PEG_Y + mToPx(R), 5, 0,
           np.pi * 2.0)

    cr.move_to(SIDE_PEG_X, -1.0 * SIDE_PEG_Y)
    cr.line_to(SIDE_PEG_X, -1.0 * SIDE_PEG_Y - mToPx(0.8))
    cr.move_to(SIDE_PEG_X, -1.0 * SIDE_PEG_Y - mToPx(R))
    cr.arc(SIDE_PEG_X, -1.0 * SIDE_PEG_Y - mToPx(R), 5, 0, np.pi * 2.0)

    cr.move_to(SIDE_PEG_X + SIDE_PEG_DX, -1.0 * SIDE_PEG_Y)
    cr.line_to(SIDE_PEG_X + SIDE_PEG_DX, -1.0 * SIDE_PEG_Y - mToPx(0.8))
    cr.move_to(SIDE_PEG_X + SIDE_PEG_DX, -1.0 * SIDE_PEG_Y - mToPx(R))
    cr.arc(SIDE_PEG_X + SIDE_PEG_DX, -1.0 * SIDE_PEG_Y - mToPx(R), 5, 0,
           np.pi * 2.0)

    cr.move_to(SIDE_PEG_X + 2.0 * SIDE_PEG_DX, -1.0 * SIDE_PEG_Y)
    cr.line_to(SIDE_PEG_X + 2.0 * SIDE_PEG_DX, -1.0 * SIDE_PEG_Y - mToPx(0.8))
    cr.move_to(SIDE_PEG_X + 2.0 * SIDE_PEG_DX, -1.0 * SIDE_PEG_Y - mToPx(R))
    cr.arc(SIDE_PEG_X + 2.0 * SIDE_PEG_DX, -1.0 * SIDE_PEG_Y - mToPx(R), 5, 0,
           np.pi * 2.0)

    cr.rectangle(X_BASE, Y_BASE - mToPx(1.41605 / 2.0), mToPx(2.43205),
                 mToPx(1.41605))
    cr.stroke()


def draw_points(cr, p, size):
    for i in range(0, len(p)):
        draw_px_cross(cr, p[i][0], p[i][1], size,
                      Color(0, np.sqrt(0.2 * i), 0))