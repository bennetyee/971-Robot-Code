// Provides a plot for debugging robot state-related issues.
import {AosPlotter} from '../../../aos/network/www/aos_plotter';
import {BLUE, BROWN, CYAN, GREEN, PINK, RED, WHITE} from '../../../aos/network/www/colors';
import * as proxy from '../../../aos/network/www/proxy';

import Connection = proxy.Connection;

const TIME = AosPlotter.TIME;
const DEFAULT_WIDTH = AosPlotter.DEFAULT_WIDTH * 2;
const DEFAULT_HEIGHT = AosPlotter.DEFAULT_HEIGHT * 3;

export function plotSuperstructure(conn: Connection, element: Element): void {
  const aosPlotter = new AosPlotter(conn);
  const goal = aosPlotter.addMessageSource(
      '/superstructure', 'y2023.control_loops.superstructure.Goal');
  const output = aosPlotter.addMessageSource(
      '/superstructure', 'y2023.control_loops.superstructure.Output');
  const status = aosPlotter.addMessageSource(
      '/superstructure', 'y2023.control_loops.superstructure.Status');
  const position = aosPlotter.addMessageSource(
      '/superstructure', 'y2023.control_loops.superstructure.Position');
  const robotState = aosPlotter.addMessageSource('/aos', 'aos.RobotState');

  const positionPlot =
      aosPlotter.addPlot(element, [DEFAULT_WIDTH, DEFAULT_HEIGHT / 2]);
  positionPlot.plot.getAxisLabels().setTitle('States');
  positionPlot.plot.getAxisLabels().setXLabel(TIME);
  positionPlot.plot.getAxisLabels().setYLabel('wonky state units');
  positionPlot.plot.setDefaultYRange([-1.0, 2.0]);

  positionPlot.addMessageLine(position, ['turret_beambreak'])
      .setColor(RED)
      .setPointSize(4.0);
  positionPlot.addMessageLine(status, ['state'])
      .setColor(PINK)
      .setPointSize(4.0);
  positionPlot.addMessageLine(status, ['flippers_open'])
      .setColor(WHITE)
      .setPointSize(1.0);
  positionPlot.addMessageLine(status, ['reseating_in_catapult'])
      .setColor(BLUE)
      .setPointSize(1.0);
  positionPlot.addMessageLine(status, ['fire'])
      .setColor(BROWN)
      .setPointSize(1.0);
  positionPlot.addMessageLine(status, ['ready_to_fire'])
      .setColor(GREEN)
      .setPointSize(1.0);
  positionPlot.addMessageLine(status, ['collided'])
      .setColor(PINK)
      .setPointSize(1.0);

  const goalPlot =
      aosPlotter.addPlot(element, [DEFAULT_WIDTH, DEFAULT_HEIGHT / 2]);
  goalPlot.plot.getAxisLabels().setTitle('Goal');
  goalPlot.plot.getAxisLabels().setXLabel(TIME);
  goalPlot.plot.getAxisLabels().setYLabel('value');
  goalPlot.plot.setDefaultYRange([-1.0, 2.0]);
  goalPlot.addMessageLine(goal, ['fire']).setColor(RED).setPointSize(1.0);
  goalPlot.addMessageLine(goal, ['auto_aim']).setColor(BLUE).setPointSize(1.0);


  const shotCountPlot =
      aosPlotter.addPlot(element, [DEFAULT_WIDTH, DEFAULT_HEIGHT / 2]);
  shotCountPlot.plot.getAxisLabels().setTitle('Shot Count');
  shotCountPlot.plot.getAxisLabels().setXLabel(TIME);
  shotCountPlot.plot.getAxisLabels().setYLabel('balls');
  shotCountPlot.plot.setDefaultYRange([-1.0, 2.0]);
  shotCountPlot.addMessageLine(status, ['shot_count'])
      .setColor(RED)
      .setPointSize(1.0);

  const intakePlot =
      aosPlotter.addPlot(element, [DEFAULT_WIDTH, DEFAULT_HEIGHT / 2]);
  intakePlot.plot.getAxisLabels().setTitle('Intake');
  intakePlot.plot.getAxisLabels().setXLabel(TIME);
  intakePlot.plot.getAxisLabels().setYLabel('wonky state units');
  intakePlot.plot.setDefaultYRange([-1.0, 2.0]);
  intakePlot.addMessageLine(status, ['intake_state'])
      .setColor(RED)
      .setPointSize(1.0);
  intakePlot.addMessageLine(position, ['intake_beambreak_front'])
      .setColor(GREEN)
      .setPointSize(4.0);
  intakePlot.addMessageLine(position, ['intake_beambreak_back'])
      .setColor(PINK)
      .setPointSize(1.0);
  intakePlot.addMessageLine(output, ['transfer_roller_voltage'])
      .setColor(BROWN)
      .setPointSize(3.0);


  const otherPlot =
      aosPlotter.addPlot(element, [DEFAULT_WIDTH, DEFAULT_HEIGHT / 2]);
  otherPlot.plot.getAxisLabels().setTitle('Position');
  otherPlot.plot.getAxisLabels().setXLabel(TIME);
  otherPlot.plot.getAxisLabels().setYLabel('rad');
  otherPlot.plot.setDefaultYRange([-1.0, 2.0]);

  otherPlot.addMessageLine(status, ['catapult', 'position'])
      .setColor(PINK)
      .setPointSize(4.0);
  otherPlot.addMessageLine(status, ['turret', 'position'])
      .setColor(WHITE)
      .setPointSize(4.0);
  otherPlot.addMessageLine(position, ['flipper_arm_left', 'encoder'])
      .setColor(BLUE)
      .setPointSize(4.0);
  otherPlot.addMessageLine(position, ['flipper_arm_right', 'encoder'])
      .setColor(CYAN)
      .setPointSize(4.0);
  otherPlot.addMessageLine(output, ['flipper_arms_voltage'])
      .setColor(BROWN)
      .setPointSize(4.0);
}
