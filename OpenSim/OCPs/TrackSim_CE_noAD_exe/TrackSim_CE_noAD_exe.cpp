#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/PlanarJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/SpatialTransform.h>
#include <OpenSim/Simulation/SimbodyEngine/CustomJoint.h>
#include <OpenSim/Common/LinearFunction.h>
#include <OpenSim/Common/Constant.h>
#include <OpenSim/Common/SimmSpline.h>
#include <OpenSim/Simulation/Model/ConditionalPathPoint.h>
#include <OpenSim/Simulation/Model/MovingPathPoint.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce_smooth.h>
//#include <adolc.h>
//#include <adolc_sparse.h>
#include <recorder.hpp>

#include <iostream>
#include <iterator>
#include <random>
#include <cassert>
#include <algorithm>
#include <vector>
#include <fstream>

using namespace SimTK;
using namespace OpenSim;

// Declare inputs/outputs of function F
/// number of vectors in inputs/outputs of function F
constexpr int n_in = 2;
constexpr int n_out = 1;
/// number of elements in input/output vectors of function F
constexpr int ndof = 21;        // degrees of freedom
constexpr int NX = ndof*2;      // states
constexpr int NU = ndof;        // controls
constexpr int NR = ndof+6+6;    // residual torques + GRFs + MOMs.

/* Function F, using templated type T
F(x,u) -> (tau)
*/

template<typename T>
T value(const Recorder& e) { return e; }

template<>
double value(const Recorder& e) { return e.getValue(); }

template<typename T>
int F_generic(const T** arg, T** res) {

    // OpenSim model
    /// bodies
    OpenSim::Model* model;
    OpenSim::Body* pelvis;
    OpenSim::Body* femur_r;
    OpenSim::Body* femur_l;
    OpenSim::Body* tibia_r;
    OpenSim::Body* tibia_l;
    OpenSim::Body* talus_r;
    OpenSim::Body* talus_l;
    OpenSim::Body* calcn_r;
    OpenSim::Body* calcn_l;
    OpenSim::Body* toes_r;
    OpenSim::Body* toes_l;
    OpenSim::Body* torso;
    /// joints
    OpenSim::CustomJoint* ground_pelvis;
    OpenSim::CustomJoint* hip_r;
    OpenSim::CustomJoint* hip_l;
    OpenSim::CustomJoint* knee_r;
    OpenSim::CustomJoint* knee_l;
    OpenSim::CustomJoint* ankle_r;
    OpenSim::CustomJoint* ankle_l;
    OpenSim::CustomJoint* subtalar_r;
    OpenSim::CustomJoint* subtalar_l;
    OpenSim::WeldJoint* mtp_r;
    OpenSim::WeldJoint* mtp_l;
    OpenSim::CustomJoint* back;
    /// contact elements
    OpenSim::HuntCrossleyForce_smooth* HC_heel_r;
    OpenSim::HuntCrossleyForce_smooth* HC_front1_r;
    OpenSim::HuntCrossleyForce_smooth* HC_front2_r;
    OpenSim::HuntCrossleyForce_smooth* HC_front3_r;
    const Vec3 locSphere_heel_r = Vec3(0.031307527581931796, 0.010435842527310599, 0);
    const Vec3 locSphere_front1_r = Vec3(0.18262724422793547, -0.015653763790965898, -0.0260896063182765);
    const Vec3 locSphere_front2_r = Vec3(0.1774093229642802, -0.015653763790965898, 0.005217921263655299);
    const Vec3 locSphere_front3_r = Vec3(0.1617555591733143, -0.01461017953823484, 0.041743370109242395);
    OpenSim::HuntCrossleyForce_smooth* HC_heel_l;
    OpenSim::HuntCrossleyForce_smooth* HC_front1_l;
    OpenSim::HuntCrossleyForce_smooth* HC_front2_l;
    OpenSim::HuntCrossleyForce_smooth* HC_front3_l;
    const Vec3 locSphere_heel_l = Vec3(0.031307527581931796, 0.010435842527310599, 0);
    const Vec3 locSphere_front1_l = Vec3(0.18262724422793547, -0.015653763790965898, 0.0260896063182765);
    const Vec3 locSphere_front2_l = Vec3(0.1774093229642802, -0.015653763790965898, -0.005217921263655299);
    const Vec3 locSphere_front3_l = Vec3(0.1617555591733143, -0.01461017953823484, -0.041743370109242395);
    /// states
    SimTK::State* state;
    /// Model
    model = new OpenSim::Model();
    /// Bodies - Definition
    pelvis = new OpenSim::Body("pelvis", 11.3124981706814, Vec3(-0.064625, 0, 0), Inertia(0.0923927863466917, 0.0923927863466917, 0.0464691170246872, 0, 0, 0));
    femur_l = new OpenSim::Body("femur_l", 8.93453939753554, Vec3(0, -0.170523, 0), Inertia(0.129411993939891, 0.0339235323920103, 0.136467315491506, 0, 0, 0));
    femur_r = new OpenSim::Body("femur_r", 8.93453939753554, Vec3(0, -0.170523, 0), Inertia(0.129411993939891, 0.0339235323920103, 0.136467315491506, 0, 0, 0));
    tibia_l = new OpenSim::Body("tibia_l", 3.56127086421001, Vec3(0, -0.180431, 0), Inertia(0.045215707042372, 0.00457539892690669, 0.045843702973516, 0, 0, 0));
    tibia_r = new OpenSim::Body("tibia_r", 3.56127086421001, Vec3(0, -0.180431, 0), Inertia(0.045215707042372, 0.00457539892690669, 0.045843702973516, 0, 0, 0));
    talus_l = new OpenSim::Body("talus_l", 0.0960558560811872, Vec3(0, 0, 0), Inertia(0.000897137044491507, 0.000897137044491507, 0.000897137044491507, 0, 0, 0));
    talus_r = new OpenSim::Body("talus_r", 0.0960558560811872, Vec3(0, 0, 0), Inertia(0.000897137044491507, 0.000897137044491507, 0.000897137044491507, 0, 0, 0));
    calcn_l = new OpenSim::Body("calcn_l", 1.20069820101484, Vec3(0.0966423, 0.0289927, 0), Inertia(0.00125599186228811, 0.00349883447351688, 0.00367826188241518, 0, 0, 0));
    calcn_r = new OpenSim::Body("calcn_r", 1.20069820101484, Vec3(0.0966423, 0.0289927, 0), Inertia(0.00125599186228811, 0.00349883447351688, 0.00367826188241518, 0, 0, 0));
    toes_l = new OpenSim::Body("toes_l", 0.208056984271851, Vec3(0.0334383, 0.00579854, 0.0169124), Inertia(8.97137044491507e-005, 0.000179427408898301, 8.97137044491507e-005, 0, 0, 0));
    toes_r = new OpenSim::Body("toes_r", 0.208056984271851, Vec3(0.0334383, 0.00579854, -0.0169124), Inertia(8.97137044491507e-005, 0.000179427408898301, 8.97137044491507e-005, 0, 0, 0));
    torso = new OpenSim::Body("torso", 32.8862592230917, Vec3(-0.0264696, 0.282343, 0), Inertia(1.10261113242884, 0.564952669074257, 1.07038153608589, 0, 0, 0));
    /// Joints - Transforms
    // Ground-Pelvis
    SpatialTransform st_ground_pelvis;
    st_ground_pelvis[0].setCoordinateNames(OpenSim::Array<std::string>("pelvis_tilt", 1, 1));
    st_ground_pelvis[0].setFunction(new LinearFunction());
    st_ground_pelvis[0].setAxis(Vec3(0, 0, 1));
    st_ground_pelvis[1].setCoordinateNames(OpenSim::Array<std::string>("pelvis_list", 1, 1));
    st_ground_pelvis[1].setFunction(new LinearFunction());
    st_ground_pelvis[1].setAxis(Vec3(1, 0, 0));
    st_ground_pelvis[2].setCoordinateNames(OpenSim::Array<std::string>("pelvis_rotation", 1, 1));
    st_ground_pelvis[2].setFunction(new LinearFunction());
    st_ground_pelvis[2].setAxis(Vec3(0, 1, 0));
    st_ground_pelvis[3].setCoordinateNames(OpenSim::Array<std::string>("pelvis_tx", 1, 1));
    st_ground_pelvis[3].setFunction(new LinearFunction());
    st_ground_pelvis[3].setAxis(Vec3(1, 0, 0));
    st_ground_pelvis[4].setCoordinateNames(OpenSim::Array<std::string>("pelvis_ty", 1, 1));
    st_ground_pelvis[4].setFunction(new LinearFunction());
    st_ground_pelvis[4].setAxis(Vec3(0, 1, 0));
    st_ground_pelvis[5].setCoordinateNames(OpenSim::Array<std::string>("pelvis_tz", 1, 1));
    st_ground_pelvis[5].setFunction(new LinearFunction());
    st_ground_pelvis[5].setAxis(Vec3(0, 0, 1));
    // Hip_l
    SpatialTransform st_hip_l;
    st_hip_l[0].setCoordinateNames(OpenSim::Array<std::string>("hip_flexion_l", 1, 1));
    st_hip_l[0].setFunction(new LinearFunction());
    st_hip_l[0].setAxis(Vec3(0, 0, 1));
    st_hip_l[1].setCoordinateNames(OpenSim::Array<std::string>("hip_adduction_l", 1, 1));
    st_hip_l[1].setFunction(new LinearFunction());
    st_hip_l[1].setAxis(Vec3(-1, 0, 0));
    st_hip_l[2].setCoordinateNames(OpenSim::Array<std::string>("hip_rotation_l", 1, 1));
    st_hip_l[2].setFunction(new LinearFunction());
    st_hip_l[2].setAxis(Vec3(0, -1, 0));
    // Hip_r
    SpatialTransform st_hip_r;
    st_hip_r[0].setCoordinateNames(OpenSim::Array<std::string>("hip_flexion_r", 1, 1));
    st_hip_r[0].setFunction(new LinearFunction());
    st_hip_r[0].setAxis(Vec3(0, 0, 1));
    st_hip_r[1].setCoordinateNames(OpenSim::Array<std::string>("hip_adduction_r", 1, 1));
    st_hip_r[1].setFunction(new LinearFunction());
    st_hip_r[1].setAxis(Vec3(1, 0, 0));
    st_hip_r[2].setCoordinateNames(OpenSim::Array<std::string>("hip_rotation_r", 1, 1));
    st_hip_r[2].setFunction(new LinearFunction());
    st_hip_r[2].setAxis(Vec3(0, 1, 0));
    // Knee_l
    SpatialTransform st_knee_l;
    st_knee_l[2].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_l", 1, 1));
    st_knee_l[2].setFunction(new LinearFunction());
    st_knee_l[2].setAxis(Vec3(0, 0, 1));
    // Knee_r
    SpatialTransform st_knee_r;
    st_knee_r[2].setCoordinateNames(OpenSim::Array<std::string>("knee_angle_r", 1, 1));
    st_knee_r[2].setFunction(new LinearFunction());
    st_knee_r[2].setAxis(Vec3(0, 0, 1));
    // Ankle_l
    SpatialTransform st_ankle_l;
    st_ankle_l[0].setCoordinateNames(OpenSim::Array<std::string>("ankle_angle_l", 1, 1));
    st_ankle_l[0].setFunction(new LinearFunction());
    st_ankle_l[0].setAxis(Vec3(0.10501355, 0.17402245, 0.97912632));
    // Ankle_r
    SpatialTransform st_ankle_r;
    st_ankle_r[0].setCoordinateNames(OpenSim::Array<std::string>("ankle_angle_r", 1, 1));
    st_ankle_r[0].setFunction(new LinearFunction());
    st_ankle_r[0].setAxis(Vec3(-0.10501355, -0.17402245, 0.97912632));
    // Subtalar_l
    SpatialTransform st_subtalar_l;
    st_subtalar_l[0].setCoordinateNames(OpenSim::Array<std::string>("subtalar_angle_l", 1, 1));
    st_subtalar_l[0].setFunction(new LinearFunction());
    st_subtalar_l[0].setAxis(Vec3(-0.78717961, -0.60474746, -0.12094949));
    // Subtalar_r
    SpatialTransform st_subtalar_r;
    st_subtalar_r[0].setCoordinateNames(OpenSim::Array<std::string>("subtalar_angle_r", 1, 1));
    st_subtalar_r[0].setFunction(new LinearFunction());
    st_subtalar_r[0].setAxis(Vec3(0.78717961, 0.60474746, -0.12094949));
    // Back
    SpatialTransform st_back;
    st_back[0].setCoordinateNames(OpenSim::Array<std::string>("lumbar_extension", 1, 1));
    st_back[0].setFunction(new LinearFunction());
    st_back[0].setAxis(Vec3(0, 0, 1));
    st_back[1].setCoordinateNames(OpenSim::Array<std::string>("lumbar_bending", 1, 1));
    st_back[1].setFunction(new LinearFunction());
    st_back[1].setAxis(Vec3(1, 0, 0));
    st_back[2].setCoordinateNames(OpenSim::Array<std::string>("lumbar_rotation", 1, 1));
    st_back[2].setFunction(new LinearFunction());
    st_back[2].setAxis(Vec3(0, 1, 0));
    /// Joints - Definition
    ground_pelvis = new CustomJoint("ground_pelvis", model->getGround(), Vec3(0), Vec3(0), *pelvis, Vec3(0), Vec3(0), st_ground_pelvis);
    hip_l = new CustomJoint("hip_l", *pelvis, Vec3(-0.0646249795478479, -0.0604202425475636, -0.0824463113771524), Vec3(0), *femur_l, Vec3(0), Vec3(0), st_hip_l);
    hip_r = new CustomJoint("hip_r", *pelvis, Vec3(-0.0646249795478479, -0.0604202425475636, 0.0824463113771524), Vec3(0), *femur_r, Vec3(0), Vec3(0), st_hip_r);
    knee_l = new CustomJoint("knee_l", *femur_l, Vec3(-0.00451371417592944, -0.397039353383509, 0), Vec3(0), *tibia_l, Vec3(0), Vec3(0), st_knee_l);
    knee_r = new CustomJoint("knee_r", *femur_r, Vec3(-0.00451371417592944, -0.397039353383509, 0), Vec3(0), *tibia_r, Vec3(0), Vec3(0), st_knee_r);
    ankle_l = new CustomJoint("ankle_l", *tibia_l, Vec3(0, -0.415562095018915, 0), Vec3(0), *talus_l, Vec3(0), Vec3(0), st_ankle_l);
    ankle_r = new CustomJoint("ankle_r", *tibia_r, Vec3(0, -0.415562095018915, 0), Vec3(0), *talus_r, Vec3(0), Vec3(0), st_ankle_r);
    subtalar_l = new CustomJoint("subtalar_l", *talus_l, Vec3(-0.0471324729629593, -0.040541464851264, -0.00765407393616235), Vec3(0), *calcn_l, Vec3(0), Vec3(0),st_subtalar_l);
    subtalar_r = new CustomJoint("subtalar_r", *talus_r, Vec3(-0.0471324729629593, -0.040541464851264, 0.00765407393616235), Vec3(0), *calcn_r, Vec3(0), Vec3(0),st_subtalar_r);
    mtp_l = new WeldJoint("mtp_l", *calcn_l, Vec3(0.172796517649726, -0.00193284695357635, -0.00104373735493123), Vec3(0), *toes_l, Vec3(0), Vec3(0));
    mtp_r = new WeldJoint("mtp_r", *calcn_r, Vec3(0.172796517649726, -0.00193284695357635, 0.00104373735493123), Vec3(0), *toes_r, Vec3(0), Vec3(0));
    back = new CustomJoint("back", *pelvis, Vec3(-0.0920471773757891, 0.0744969707659068, 0), Vec3(0), *torso, Vec3(0), Vec3(0), st_back);
    /// bodies and joints
    model->addBody(pelvis);		    model->addJoint(ground_pelvis);
    model->addBody(femur_l);		model->addJoint(hip_l);
    model->addBody(femur_r);		model->addJoint(hip_r);
    model->addBody(tibia_l);		model->addJoint(knee_l);
    model->addBody(tibia_r);		model->addJoint(knee_r);
    model->addBody(talus_l);		model->addJoint(ankle_l);
    model->addBody(talus_r);		model->addJoint(ankle_r);
    model->addBody(calcn_l);		model->addJoint(subtalar_l);
    model->addBody(calcn_r);		model->addJoint(subtalar_r);
    model->addBody(toes_l);		    model->addJoint(mtp_l);
    model->addBody(toes_r);		    model->addJoint(mtp_r);
    model->addBody(torso);          model->addJoint(back);
    /// Contact elements
    osim_double_adouble radiusSphere_heel = 0.035;
    osim_double_adouble radiusSphere_front1 = 0.02;
    osim_double_adouble radiusSphere_front2 = 0.015;
    osim_double_adouble radiusSphere_front3 = 0.015;
    osim_double_adouble stiffness_heel = 3067776;
    osim_double_adouble stiffness_front = 1067776;
    osim_double_adouble dissipation = 2.0;
    osim_double_adouble staticFriction = 0.8;
    osim_double_adouble dynamicFriction = 0.8;
    osim_double_adouble viscousFriction = 0.5;
    osim_double_adouble transitionVelocity = 0.2;
    Vec3 normal = Vec3(0, 1, 0);
    osim_double_adouble offset = 0;
    /// Left - Definition
    HC_heel_l = new HuntCrossleyForce_smooth("sphere_heel_l", "calcn_l", locSphere_heel_l, radiusSphere_heel,
        stiffness_heel, dissipation, staticFriction, dynamicFriction, viscousFriction, transitionVelocity, normal, offset);
    HC_front1_l = new HuntCrossleyForce_smooth("sphere_front1_l", "calcn_l", locSphere_front1_l, radiusSphere_front1,
        stiffness_front, dissipation, staticFriction, dynamicFriction, viscousFriction, transitionVelocity, normal, offset);
    HC_front2_l = new HuntCrossleyForce_smooth("sphere_front2_l", "calcn_l", locSphere_front2_l, radiusSphere_front2,
        stiffness_front, dissipation, staticFriction, dynamicFriction, viscousFriction, transitionVelocity, normal, offset);
    HC_front3_l = new HuntCrossleyForce_smooth("sphere_front3_l", "calcn_l", locSphere_front3_l, radiusSphere_front3,
        stiffness_front, dissipation, staticFriction, dynamicFriction, viscousFriction, transitionVelocity, normal, offset);
    model->addComponent(HC_heel_l);
    HC_heel_l->connectSocket_body_sphere(*calcn_l);
    model->addComponent(HC_front1_l);
    HC_front1_l->connectSocket_body_sphere(*calcn_l);
    model->addComponent(HC_front2_l);
    HC_front2_l->connectSocket_body_sphere(*calcn_l);
    model->addComponent(HC_front3_l);
    HC_front3_l->connectSocket_body_sphere(*calcn_l);
    /// Right - Definition
    HC_heel_r = new HuntCrossleyForce_smooth("sphere_heel_r", "calcn_r", locSphere_heel_r, radiusSphere_heel,
        stiffness_heel, dissipation, staticFriction, dynamicFriction, viscousFriction, transitionVelocity, normal, offset);
    HC_front1_r = new HuntCrossleyForce_smooth("sphere_front1_r", "calcn_r", locSphere_front1_r, radiusSphere_front1,
        stiffness_front, dissipation, staticFriction, dynamicFriction, viscousFriction, transitionVelocity, normal, offset);
    HC_front2_r = new HuntCrossleyForce_smooth("sphere_front2_r", "calcn_r", locSphere_front2_r, radiusSphere_front2,
        stiffness_front, dissipation, staticFriction, dynamicFriction, viscousFriction, transitionVelocity, normal, offset);
    HC_front3_r = new HuntCrossleyForce_smooth("sphere_front3_r", "calcn_r", locSphere_front3_r, radiusSphere_front3,
        stiffness_front, dissipation, staticFriction, dynamicFriction, viscousFriction, transitionVelocity, normal, offset);
    model->addComponent(HC_heel_r);
    HC_heel_r->connectSocket_body_sphere(*calcn_r);
    model->addComponent(HC_front1_r);
    HC_front1_r->connectSocket_body_sphere(*calcn_r);
    model->addComponent(HC_front2_r);
    HC_front2_r->connectSocket_body_sphere(*calcn_r);
    model->addComponent(HC_front3_r);
    HC_front3_r->connectSocket_body_sphere(*calcn_r);
    /// Initialize  system and state.
    state = new State(model->initSystem());

    // Read inputs
    std::vector<T> x(arg[0], arg[0] + NX);
    std::vector<T> u(arg[1], arg[1] + NU);

    // States and controls
    T ua[NU]; /// joint accelerations (Qdotdots) - controls
    Vector QsUs(NX); /// joint positions (Qs) and velocities (Us) - states

    // Assign inputs to model variables
    for (int i = 0; i < NX; ++i) QsUs[i] = x[i];    // states
    for (int i = 0; i < 12; ++i) ua[i] = u[i];      // controls
    ua[12] = u[18]; // 12 Simbody (lumbar-ext) is 18 OpenSim
    ua[13] = u[19]; // 13 Simbody (lumbar-bend) is 19 OpenSim
    ua[14] = u[20]; // 14 Simbody (lumbar-rot) is 20 OpenSim
    ua[15] = u[12]; // 15 Simbody (knee-angle-l) is 12 OpenSim
    ua[16] = u[13]; // 16 Simbody (knee-angle-r) is 13 OpenSim
    ua[17] = u[14]; // 17 Simbody (ankle-angle-l) is 14 OpenSim
    ua[18] = u[15]; // 18 Simbody (ankle-angle-l) is 15 OpenSim
    ua[19] = u[16]; // 19 Simbody (subtalar-angle-l) is 16 OpenSim
    ua[20] = u[17]; // 20 Simbody (subtalar-angle-l) is 17 OpenSim

    model->setStateVariableValues(*state, QsUs);
    model->realizeVelocity(*state);

    // Residual forces
    /// appliedMobilityForces (# mobilities)
    Vector appliedMobilityForces(ndof);
    appliedMobilityForces.setToZero();
    /// appliedBodyForces (# bodies + ground)
    Vector_<SpatialVec> appliedBodyForces;
    int nbodies = model->getBodySet().getSize() + 1; // including ground
    appliedBodyForces.resize(nbodies);
    appliedBodyForces.setToZero();
    /// Gravity
    Vec3 gravity(0);
    gravity[1] = -9.81;
    /// Weight
    for (int i = 0; i < model->getBodySet().getSize(); ++i) {
        model->getMatterSubsystem().addInStationForce(*state,
            model->getBodySet().get(i).getMobilizedBodyIndex(),
            model->getBodySet().get(i).getMassCenter(),
            model->getBodySet().get(i).getMass()*gravity, appliedBodyForces);
    }
    /// Contact forces
    // Right
    Array<osim_double_adouble> Force_values_heel_r = HC_heel_r->getRecordValues(*state);
    Array<osim_double_adouble> Force_values_front1_r = HC_front1_r->getRecordValues(*state);
    Array<osim_double_adouble> Force_values_front2_r = HC_front2_r->getRecordValues(*state);
    Array<osim_double_adouble> Force_values_front3_r = HC_front3_r->getRecordValues(*state);
    SpatialVec GRF_heel_r;
    GRF_heel_r[0] = Vec3(Force_values_heel_r[9], Force_values_heel_r[10], Force_values_heel_r[11]);
    GRF_heel_r[1] = Vec3(Force_values_heel_r[6], Force_values_heel_r[7], Force_values_heel_r[8]);
    SpatialVec GRF_front1_r;
    GRF_front1_r[0] = Vec3(Force_values_front1_r[9], Force_values_front1_r[10], Force_values_front1_r[11]);
    GRF_front1_r[1] = Vec3(Force_values_front1_r[6], Force_values_front1_r[7], Force_values_front1_r[8]);
    SpatialVec GRF_front2_r;
    GRF_front2_r[0] = Vec3(Force_values_front2_r[9], Force_values_front2_r[10], Force_values_front2_r[11]);
    GRF_front2_r[1] = Vec3(Force_values_front2_r[6], Force_values_front2_r[7], Force_values_front2_r[8]);
    SpatialVec GRF_front3_r;
    GRF_front3_r[0] = Vec3(Force_values_front3_r[9], Force_values_front3_r[10], Force_values_front3_r[11]);
    GRF_front3_r[1] = Vec3(Force_values_front3_r[6], Force_values_front3_r[7], Force_values_front3_r[8]);
    int nfoot_r = model->getBodySet().get("calcn_r").getMobilizedBodyIndex();
    appliedBodyForces[nfoot_r] = appliedBodyForces[nfoot_r] + GRF_heel_r + GRF_front1_r + GRF_front2_r + GRF_front3_r;
    // Left
    Array<osim_double_adouble> Force_values_heel_l = HC_heel_l->getRecordValues(*state);
    Array<osim_double_adouble> Force_values_front1_l = HC_front1_l->getRecordValues(*state);
    Array<osim_double_adouble> Force_values_front2_l = HC_front2_l->getRecordValues(*state);
    Array<osim_double_adouble> Force_values_front3_l = HC_front3_l->getRecordValues(*state);
    SpatialVec GRF_heel_l;
    GRF_heel_l[0] = Vec3(Force_values_heel_l[9], Force_values_heel_l[10], Force_values_heel_l[11]);
    GRF_heel_l[1] = Vec3(Force_values_heel_l[6], Force_values_heel_l[7], Force_values_heel_l[8]);
    SpatialVec GRF_front1_l;
    GRF_front1_l[0] = Vec3(Force_values_front1_l[9], Force_values_front1_l[10], Force_values_front1_l[11]);
    GRF_front1_l[1] = Vec3(Force_values_front1_l[6], Force_values_front1_l[7], Force_values_front1_l[8]);
    SpatialVec GRF_front2_l;
    GRF_front2_l[0] = Vec3(Force_values_front2_l[9], Force_values_front2_l[10], Force_values_front2_l[11]);
    GRF_front2_l[1] = Vec3(Force_values_front2_l[6], Force_values_front2_l[7], Force_values_front2_l[8]);
    SpatialVec GRF_front3_l;
    GRF_front3_l[0] = Vec3(Force_values_front3_l[9], Force_values_front3_l[10], Force_values_front3_l[11]);
    GRF_front3_l[1] = Vec3(Force_values_front3_l[6], Force_values_front3_l[7], Force_values_front3_l[8]);
    int nfoot_l = model->getBodySet().get("calcn_l").getMobilizedBodyIndex();
    appliedBodyForces[nfoot_l] = appliedBodyForces[nfoot_l] + GRF_heel_l + GRF_front1_l + GRF_front2_l + GRF_front3_l;
    // Ground reaction forces
    SpatialVec GRF_r = GRF_heel_r + GRF_front1_r + GRF_front2_r + GRF_front3_r;
    SpatialVec GRF_l = GRF_heel_l + GRF_front1_l + GRF_front2_l + GRF_front3_l;

    /// knownUdot
    Vector knownUdot(ndof);
    knownUdot.setToZero();
    for (int i = 0; i < ndof; ++i) {
        knownUdot[i] = ua[i];
    }
    // Residual forces
    Vector residualMobilityForces(ndof);
    residualMobilityForces.setToZero();
    model->getMatterSubsystem().calcResidualForceIgnoringConstraints(*state,
        appliedMobilityForces, appliedBodyForces, knownUdot,
        residualMobilityForces);

    // Calculate contact torques about the ground origin
    // Step: calculate contact point positions in body frames
    //osim_double_adouble radiusSphere_heel = 0.035;
    //osim_double_adouble radiusSphere_front1 = 0.02;
    //osim_double_adouble radiusSphere_front2 = 0.015;
    //osim_double_adouble radiusSphere_front3 = 0.015;
    //Vec3 normal = Vec3(0, 1, 0);
    // Heel l
    Vec3 pos_InGround_HC_heel_l = calcn_l->findStationLocationInGround(*state, locSphere_heel_l);
    Vec3 contactPointPos_InGround_HC_heel_l = pos_InGround_HC_heel_l - radiusSphere_heel*normal;
    Vec3 contactPointPos_InBody_HC_heel_l = model->getGround().findStationLocationInAnotherFrame(*state, contactPointPos_InGround_HC_heel_l, *calcn_l);
    // Front1 l
    Vec3 pos_InGround_HC_front1_l = calcn_l->findStationLocationInGround(*state, locSphere_front1_l);
    Vec3 contactPointPos_InGround_HC_front1_l = pos_InGround_HC_front1_l - radiusSphere_front1*normal;
    Vec3 contactPointPos_InBody_HC_front1_l = model->getGround().findStationLocationInAnotherFrame(*state, contactPointPos_InGround_HC_front1_l, *calcn_l);
    // Front2 l
    Vec3 pos_InGround_HC_front2_l = calcn_l->findStationLocationInGround(*state, locSphere_front2_l);
    Vec3 contactPointPos_InGround_HC_front2_l = pos_InGround_HC_front2_l - radiusSphere_front2*normal;
    Vec3 contactPointPos_InBody_HC_front2_l = model->getGround().findStationLocationInAnotherFrame(*state, contactPointPos_InGround_HC_front2_l, *calcn_l);
    // Front3 l
    Vec3 pos_InGround_HC_front3_l = calcn_l->findStationLocationInGround(*state, locSphere_front3_l);
    Vec3 contactPointPos_InGround_HC_front3_l = pos_InGround_HC_front3_l - radiusSphere_front3*normal;
    Vec3 contactPointPos_InBody_HC_front3_l = model->getGround().findStationLocationInAnotherFrame(*state, contactPointPos_InGround_HC_front3_l, *calcn_l);
    // Heel r
    Vec3 pos_InGround_HC_heel_r = calcn_r->findStationLocationInGround(*state, locSphere_heel_r);
    Vec3 contactPointPos_InGround_HC_heel_r = pos_InGround_HC_heel_r - radiusSphere_heel*normal;
    Vec3 contactPointPos_InBody_HC_heel_r = model->getGround().findStationLocationInAnotherFrame(*state, contactPointPos_InGround_HC_heel_r, *calcn_r);
    // Front1 r
    Vec3 pos_InGround_HC_front1_r = calcn_r->findStationLocationInGround(*state, locSphere_front1_r);
    Vec3 contactPointPos_InGround_HC_front1_r = pos_InGround_HC_front1_r - radiusSphere_front1*normal;
    Vec3 contactPointPos_InBody_HC_front1_r = model->getGround().findStationLocationInAnotherFrame(*state, contactPointPos_InGround_HC_front1_r, *calcn_r);
    // Front2 r
    Vec3 pos_InGround_HC_front2_r = calcn_r->findStationLocationInGround(*state, locSphere_front2_r);
    Vec3 contactPointPos_InGround_HC_front2_r = pos_InGround_HC_front2_r - radiusSphere_front2*normal;
    Vec3 contactPointPos_InBody_HC_front2_r = model->getGround().findStationLocationInAnotherFrame(*state, contactPointPos_InGround_HC_front2_r, *calcn_r);
    // Front3 r
    Vec3 pos_InGround_HC_front3_r = calcn_r->findStationLocationInGround(*state, locSphere_front3_r);
    Vec3 contactPointPos_InGround_HC_front3_r = pos_InGround_HC_front3_r - radiusSphere_front3*normal;
    Vec3 contactPointPos_InBody_HC_front3_r = model->getGround().findStationLocationInAnotherFrame(*state, contactPointPos_InGround_HC_front3_r, *calcn_r);
    // Contact forces
    Vec3 AppliedPointForce_heel_l = GRF_heel_l[1];
    Vec3 AppliedPointForce_front1_l = GRF_front1_l[1];
    Vec3 AppliedPointForce_front2_l = GRF_front2_l[1];
    Vec3 AppliedPointForce_front3_l = GRF_front3_l[1];
    Vec3 AppliedPointForce_heel_r = GRF_heel_r[1];
    Vec3 AppliedPointForce_front1_r = GRF_front1_r[1];
    Vec3 AppliedPointForce_front2_r = GRF_front2_r[1];
    Vec3 AppliedPointForce_front3_r = GRF_front3_r[1];

    // Approach 1
    // Calculate transforms
    SimTK::Transform TR_GB_calcn_l = calcn_l->getMobilizedBody().getBodyTransform(*state);
    SimTK::Transform TR_GB_calcn_r = calcn_r->getMobilizedBody().getBodyTransform(*state);
    // Calculate torques
    Vec3 AppliedPointTorque_heel_l, AppliedPointTorque_front1_l, AppliedPointTorque_front2_l, AppliedPointTorque_front3_l;
    Vec3 AppliedPointTorque_heel_r, AppliedPointTorque_front1_r, AppliedPointTorque_front2_r, AppliedPointTorque_front3_r;
    AppliedPointTorque_heel_l = (TR_GB_calcn_l*contactPointPos_InBody_HC_heel_l) % AppliedPointForce_heel_l;
    AppliedPointTorque_front1_l = (TR_GB_calcn_l*contactPointPos_InBody_HC_front1_l) % AppliedPointForce_front1_l;
    AppliedPointTorque_front2_l = (TR_GB_calcn_l*contactPointPos_InBody_HC_front2_l) % AppliedPointForce_front2_l;
    AppliedPointTorque_front3_l = (TR_GB_calcn_l*contactPointPos_InBody_HC_front3_l) % AppliedPointForce_front3_l;
    AppliedPointTorque_heel_r = (TR_GB_calcn_r*contactPointPos_InBody_HC_heel_r) % AppliedPointForce_heel_r;
    AppliedPointTorque_front1_r = (TR_GB_calcn_r*contactPointPos_InBody_HC_front1_r) % AppliedPointForce_front1_r;
    AppliedPointTorque_front2_r = (TR_GB_calcn_r*contactPointPos_InBody_HC_front2_r) % AppliedPointForce_front2_r;
    AppliedPointTorque_front3_r = (TR_GB_calcn_r*contactPointPos_InBody_HC_front3_r) % AppliedPointForce_front3_r;
    Vec3 MOM_l, MOM_r;
    MOM_l = AppliedPointTorque_heel_l + AppliedPointTorque_front1_l + AppliedPointTorque_front2_l + AppliedPointTorque_front3_l;
    MOM_r = AppliedPointTorque_heel_r + AppliedPointTorque_front1_r + AppliedPointTorque_front2_r + AppliedPointTorque_front3_r;

    //Vec3 MOM_B_l, MOM_B_r;
    //MOM_B_l = ~TR_GB_calcn_l*MOM_l;
    //MOM_B_r = ~TR_GB_calcn_r*MOM_r;

    //// Approach 2
    //// Calculate COP
    //Vec3 COP_r, COP_l;
    //COP_r.setToZero();
    //COP_l.setToZero();
    //COP_r[0] = (contactPointPos_InGround_HC_heel_r[0] * AppliedPointForce_heel_r[1]) - (contactPointPos_InGround_HC_heel_r[1] * AppliedPointForce_heel_r[0])
    //    + (contactPointPos_InGround_HC_front1_r[0] * AppliedPointForce_front1_r[1]) - (contactPointPos_InGround_HC_front1_r[1] * AppliedPointForce_front1_r[0])
    //    + (contactPointPos_InGround_HC_front2_r[0] * AppliedPointForce_front2_r[1]) - (contactPointPos_InGround_HC_front2_r[1] * AppliedPointForce_front2_r[0])
    //    + (contactPointPos_InGround_HC_front3_r[0] * AppliedPointForce_front3_r[1]) - (contactPointPos_InGround_HC_front3_r[1] * AppliedPointForce_front3_r[0]);
    //COP_r[0] /= (AppliedPointForce_heel_r[1] + AppliedPointForce_front1_r[1] + AppliedPointForce_front2_r[1] + AppliedPointForce_front3_r[1]);

    //COP_r[2] = -((contactPointPos_InGround_HC_heel_r[1] * AppliedPointForce_heel_r[2]) - (contactPointPos_InGround_HC_heel_r[2] * AppliedPointForce_heel_r[1])
    //    + (contactPointPos_InGround_HC_front1_r[1] * AppliedPointForce_front1_r[2]) - (contactPointPos_InGround_HC_front1_r[2] * AppliedPointForce_front1_r[1])
    //    + (contactPointPos_InGround_HC_front2_r[1] * AppliedPointForce_front2_r[2]) - (contactPointPos_InGround_HC_front2_r[2] * AppliedPointForce_front2_r[1])
    //    + (contactPointPos_InGround_HC_front3_r[1] * AppliedPointForce_front3_r[2]) - (contactPointPos_InGround_HC_front3_r[2] * AppliedPointForce_front3_r[1]));
    //COP_r[2] /= (AppliedPointForce_heel_r[1] + AppliedPointForce_front1_r[1] + AppliedPointForce_front2_r[1] + AppliedPointForce_front3_r[1]);

    //COP_l[0] = (contactPointPos_InGround_HC_heel_l[0] * AppliedPointForce_heel_l[1]) - (contactPointPos_InGround_HC_heel_l[1] * AppliedPointForce_heel_l[0])
    //    + (contactPointPos_InGround_HC_front1_l[0] * AppliedPointForce_front1_l[1]) - (contactPointPos_InGround_HC_front1_l[1] * AppliedPointForce_front1_l[0])
    //    + (contactPointPos_InGround_HC_front2_l[0] * AppliedPointForce_front2_l[1]) - (contactPointPos_InGround_HC_front2_l[1] * AppliedPointForce_front2_l[0])
    //    + (contactPointPos_InGround_HC_front3_l[0] * AppliedPointForce_front3_l[1]) - (contactPointPos_InGround_HC_front3_l[1] * AppliedPointForce_front3_l[0]);
    //COP_l[0] /= (AppliedPointForce_heel_l[1] + AppliedPointForce_front1_l[1] + AppliedPointForce_front2_l[1] + AppliedPointForce_front3_l[1]);

    //COP_l[2] = -((contactPointPos_InGround_HC_heel_l[1] * AppliedPointForce_heel_l[2]) - (contactPointPos_InGround_HC_heel_l[2] * AppliedPointForce_heel_l[1])
    //    + (contactPointPos_InGround_HC_front1_l[1] * AppliedPointForce_front1_l[2]) - (contactPointPos_InGround_HC_front1_l[2] * AppliedPointForce_front1_l[1])
    //    + (contactPointPos_InGround_HC_front2_l[1] * AppliedPointForce_front2_l[2]) - (contactPointPos_InGround_HC_front2_l[2] * AppliedPointForce_front2_l[1])
    //    + (contactPointPos_InGround_HC_front3_l[1] * AppliedPointForce_front3_l[2]) - (contactPointPos_InGround_HC_front3_l[2] * AppliedPointForce_front3_l[1]));
    //COP_l[2] /= (AppliedPointForce_heel_l[1] + AppliedPointForce_front1_l[1] + AppliedPointForce_front2_l[1] + AppliedPointForce_front3_l[1]);
    //// Calculate torques applied at the COP
    //osim_double_adouble TY_r = (contactPointPos_InGround_HC_heel_r[2] * AppliedPointForce_heel_r[0]) - (contactPointPos_InGround_HC_heel_r[0] * AppliedPointForce_heel_r[2])
    //    + (contactPointPos_InGround_HC_front1_r[2] * AppliedPointForce_front1_r[0]) - (contactPointPos_InGround_HC_front1_r[0] * AppliedPointForce_front1_r[2])
    //    + (contactPointPos_InGround_HC_front2_r[2] * AppliedPointForce_front2_r[0]) - (contactPointPos_InGround_HC_front2_r[0] * AppliedPointForce_front2_r[2])
    //    + (contactPointPos_InGround_HC_front3_r[2] * AppliedPointForce_front3_r[0]) - (contactPointPos_InGround_HC_front3_r[0] * AppliedPointForce_front3_r[2])
    //    - COP_r[2] * (AppliedPointForce_heel_r[0] + AppliedPointForce_front1_r[0] + AppliedPointForce_front2_r[0] + AppliedPointForce_front3_r[0])
    //    + COP_r[0] * (AppliedPointForce_heel_r[2] + AppliedPointForce_front1_r[2] + AppliedPointForce_front2_r[2] + AppliedPointForce_front3_r[2]);

    //osim_double_adouble TY_l = (contactPointPos_InGround_HC_heel_l[2] * AppliedPointForce_heel_l[0]) - (contactPointPos_InGround_HC_heel_l[0] * AppliedPointForce_heel_l[2])
    //    + (contactPointPos_InGround_HC_front1_l[2] * AppliedPointForce_front1_l[0]) - (contactPointPos_InGround_HC_front1_l[0] * AppliedPointForce_front1_l[2])
    //    + (contactPointPos_InGround_HC_front2_l[2] * AppliedPointForce_front2_l[0]) - (contactPointPos_InGround_HC_front2_l[0] * AppliedPointForce_front2_l[2])
    //    + (contactPointPos_InGround_HC_front3_l[2] * AppliedPointForce_front3_l[0]) - (contactPointPos_InGround_HC_front3_l[0] * AppliedPointForce_front3_l[2])
    //    - COP_l[2] * (AppliedPointForce_heel_l[0] + AppliedPointForce_front1_l[0] + AppliedPointForce_front2_l[0] + AppliedPointForce_front3_l[0])
    //    + COP_l[0] * (AppliedPointForce_heel_l[2] + AppliedPointForce_front1_l[2] + AppliedPointForce_front2_l[2] + AppliedPointForce_front3_l[2]);
    //// Calculate torques applied at the ground frame origin
    //Vec3 MOM2_r, MOM2_l;
    //MOM2_r.setToZero();
    //MOM2_l.setToZero();
    //MOM2_r[0] = COP_r[1] * (AppliedPointForce_heel_r[2] + AppliedPointForce_front1_r[2] + AppliedPointForce_front2_r[2] + AppliedPointForce_front3_r[2])
    //    - COP_r[2] * (AppliedPointForce_heel_r[1] + AppliedPointForce_front1_r[1] + AppliedPointForce_front2_r[1] + AppliedPointForce_front3_r[1]);
    //MOM2_r[1] = COP_r[2] * (AppliedPointForce_heel_r[0] + AppliedPointForce_front1_r[0] + AppliedPointForce_front2_r[0] + AppliedPointForce_front3_r[0])
    //    - COP_r[0] * (AppliedPointForce_heel_r[2] + AppliedPointForce_front1_r[2] + AppliedPointForce_front2_r[2] + AppliedPointForce_front3_r[2]) + TY_r;
    //MOM2_r[2] = COP_r[0] * (AppliedPointForce_heel_r[1] + AppliedPointForce_front1_r[1] + AppliedPointForce_front2_r[1] + AppliedPointForce_front3_r[1])
    //    - COP_r[1] * (AppliedPointForce_heel_r[0] + AppliedPointForce_front1_r[0] + AppliedPointForce_front2_r[0] + AppliedPointForce_front3_r[0]);
    //MOM2_l[0] = COP_l[1] * (AppliedPointForce_heel_l[2] + AppliedPointForce_front1_l[2] + AppliedPointForce_front2_l[2] + AppliedPointForce_front3_l[2])
    //    - COP_l[2] * (AppliedPointForce_heel_l[1] + AppliedPointForce_front1_l[1] + AppliedPointForce_front2_l[1] + AppliedPointForce_front3_l[1]);
    //MOM2_l[1] = COP_l[2] * (AppliedPointForce_heel_l[0] + AppliedPointForce_front1_l[0] + AppliedPointForce_front2_l[0] + AppliedPointForce_front3_l[0])
    //    - COP_l[0] * (AppliedPointForce_heel_l[2] + AppliedPointForce_front1_l[2] + AppliedPointForce_front2_l[2] + AppliedPointForce_front3_l[2]) + TY_l;
    //MOM2_l[2] = COP_l[0] * (AppliedPointForce_heel_l[1] + AppliedPointForce_front1_l[1] + AppliedPointForce_front2_l[1] + AppliedPointForce_front3_l[1])
    //    - COP_l[1] * (AppliedPointForce_heel_l[0] + AppliedPointForce_front1_l[0] + AppliedPointForce_front2_l[0] + AppliedPointForce_front3_l[0]);

    ////Approach 1 and 2 give same torques
    //std::ofstream myfile1;
    //myfile1.open("test1.txt");
    //myfile1 << MOM2_l << "  ";
    //myfile1 << MOM_l << "  ";
    //myfile1 << MOM2_r << "  ";
    //myfile1 << MOM_r << "  ";
    //myfile1.close();

     //CasADi may not always request all outputs
     //if res[i] is a null pointer, this means that output i is not required
    int nc = 3; // # components in Vec3
    if (res[0]) {
        for (int i = 0; i < 12; ++i) {
            res[0][i] = value<T>(residualMobilityForces[i]); // residual torques
        }
        res[0][12] = value<T>(residualMobilityForces[15]); // order adjusted since order Simbody is different than order OpenSim
        res[0][13] = value<T>(residualMobilityForces[16]);
        res[0][14] = value<T>(residualMobilityForces[17]);
        res[0][15] = value<T>(residualMobilityForces[18]);
        res[0][16] = value<T>(residualMobilityForces[19]);
        res[0][17] = value<T>(residualMobilityForces[20]);
        res[0][18] = value<T>(residualMobilityForces[12]);
        res[0][19] = value<T>(residualMobilityForces[13]);
        res[0][20] = value<T>(residualMobilityForces[14]);
        for (int i = 0; i < nc; ++i) {
            res[0][i + ndof] = value<T>(GRF_r[1][i]); // GRF_r
        }
        for (int i = 0; i < nc; ++i) {
            res[0][i + ndof + nc] = value<T>(GRF_l[1][i]); // GRF_l
        }
        for (int i = 0; i < nc; ++i) {
            res[0][i + ndof + nc + nc] = value<T>(MOM_r[i]); // MOM_r
        }
        for (int i = 0; i < nc; ++i) {
            res[0][i + ndof + nc + nc + nc] = value<T>(MOM_l[i]); // MOM_l
        }
    }
    return 0;
}

int main() {

    Recorder x[NX];
    Recorder u[NU];
    Recorder tau[NR];

    for (int i = 0; i < NX; ++i) x[i] <<= 0;
    for (int i = 0; i < NU; ++i) u[i] <<= 0;

    const Recorder* Recorder_arg[n_in] = { x,u };
    Recorder* Recorder_res[n_out] = { tau };

    F_generic<Recorder>(Recorder_arg, Recorder_res);

    double res[NR];
    for (int i = 0; i < NR; ++i) Recorder_res[0][i] >>= res[i];

    Recorder::stop_recording();

    return 0;
}
