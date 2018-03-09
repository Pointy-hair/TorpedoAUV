package com.github.rosjava.android_apps.teleop;

import android.util.Log;

import org.jboss.netty.handler.codec.marshalling.MarshallingEncoder;
import org.ros.concurrent.CancellableLoop;
import org.ros.message.Duration;
import org.ros.message.MessageListener;
import org.ros.message.Time;
import org.ros.namespace.GraphName;
import org.ros.node.AbstractNodeMain;
import org.ros.node.ConnectedNode;
import org.ros.node.parameter.ParameterTree;
import org.ros.node.topic.Publisher;
import org.ros.node.topic.Subscriber;
import org.ros.rosjava_geometry.Quaternion;
import org.ros.rosjava_geometry.Transform;
import org.ros.rosjava_geometry.Vector3;

import std_msgs.Float64MultiArray;
import std_msgs.Int32;
import visualization_msgs.Marker;

public class PlannerNode extends AbstractNodeMain {
    private Planner rov_planner = new Planner();

    double[] state_reference = new double[12];

    private int status_system;
    private int status_planner;

    private Time time_current;
    private Time time_status_system;
    private Time time_state_pose;
    private Duration timeout_status_system = new Duration(0.04);
    private Duration timeout_state_pose = new Duration(0.04);

    @Override
    public GraphName getDefaultNodeName() {
        return GraphName.of("PlannerNode");
    }

    @Override
    public void onStart(final ConnectedNode connectedNode) {
        // Define system connections
        final Publisher<Int32> status_planner_pub = connectedNode.newPublisher("status_controller", Int32._TYPE);
        final Subscriber<Int32> status_system_sub = connectedNode.newSubscriber("status_system", Int32._TYPE);
        final ParameterTree param_tree = connectedNode.getParameterTree();
        // Define data connections
        final Publisher<Float64MultiArray> state_reference_pub = connectedNode.newPublisher("state_reference", Float64MultiArray._TYPE);
        final Subscriber<geometry_msgs.Transform> state_pose_pub = connectedNode.newSubscriber("state_pose", geometry_msgs.Transform._TYPE);
        // Define visualization markers
        final Publisher<Marker> marker_path_position_pub = connectedNode.newPublisher("marker_path_position", Marker._TYPE);
        final Publisher<Marker> marker_path_attitude_pub = connectedNode.newPublisher("marker_path_attitude", Marker._TYPE);
        final Publisher<Marker> marker_pose_current_pub = connectedNode.newPublisher("marker_pose_current", Marker._TYPE);
        final Publisher<Marker> marker_pose_desired_pub = connectedNode.newPublisher("marker_pose_deisred", Marker._TYPE);
        // Main loop
        connectedNode.executeCancellableLoop(new CancellableLoop() {
            Int32 status_planner_msg = status_planner_pub.newMessage();

            @Override protected void setup() {
                time_status_system = connectedNode.getCurrentTime();
                time_state_pose = connectedNode.getCurrentTime();
                status_system = 0;
            }

            @Override
            protected void loop() throws InterruptedException {
                // Check system state compliance
                if (status_system <= 2) {
                    status_planner |= 1;
                } else {
                    status_planner &= ~1;
                }

                // Check timeouts
                time_current = connectedNode.getCurrentTime();
                if (time_current.compareTo(time_status_system.add(timeout_status_system)) == 1) {
                    if(status_system <= 0){Log.e("ROV_ERROR", "Planner node: Timeout on system state");}
                    status_planner |= 2;
                } else {status_planner &= ~2;}
                if (time_current.compareTo(time_state_pose.add(timeout_state_pose)) == 1) {
                    if(status_system <= 0){Log.e("ROV_ERROR", "Planner node: Timeout on state pose");}
                    status_planner |= 2;
                } else {status_planner &= ~2;}

                // Check if all data/params filled
                if (!rov_planner.isReady()){
                    status_planner |= 4;
                }else{
                    status_planner &= ~4;
                }

                // Check if the system is ready to proceed
                //TODO: check if we are close to the start position

                // Publish status
                status_planner_msg.setData(status_planner);
                status_planner_pub.publish(status_planner_msg);
                Thread.sleep(10);
            }
        });

        // Pose callback
        state_pose_pub.addMessageListener(new MessageListener<geometry_msgs.Transform>() {
            @Override
            public void onNewMessage(geometry_msgs.Transform pose_msg) {
                if((status_planner&127) == 0) { // We should be good to output
                    Float64MultiArray state_reference_msg = state_reference_pub.newMessage();
                    if (status_system <= 3) {
                        state_reference = new double[12];
                        state_reference_pub.publish(state_reference_msg);
                    } else {
                        // Setup planner input
                        Vector3 position = new Vector3(pose_msg.getTranslation().getX(), pose_msg.getTranslation().getY(), pose_msg.getTranslation().getZ());
                        Quaternion attitude = new Quaternion(pose_msg.getRotation().getX(), pose_msg.getRotation().getY(), pose_msg.getRotation().getZ(), pose_msg.getRotation().getW());
                        Transform pose = new Transform(position, attitude);
                        // Update pose, attempt reference calculation
                        rov_planner.setPose(pose);
                        if (rov_planner.calculateReference()) {
                            state_reference = rov_planner.getStateReference();
                            state_reference_msg.setData(state_reference);
                            state_reference_pub.publish(state_reference_msg);
                        } else {
                            Log.d("ROV_ERROR", "Planner: Failed to calculate controller reference");
                        }
                    }
                }
                // else{} // We shouldn't be outputting
            }
        });

        // System state callback
        status_system_sub.addMessageListener(new MessageListener<Int32>() {
            @Override
            public void onNewMessage(Int32 status_system_msg) {
                time_status_system = connectedNode.getCurrentTime();
                status_system = status_system_msg.getData();
            }
        });
    }
}
