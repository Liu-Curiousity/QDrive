/**
 * @brief 		filters implementation
 * @detail
 * @author 	    Haoqi Liu
 * @date        25-4-11
 * @version 	V3.0.0
 * @note
 * @warning
 * @par 		history
                V1.0.0 on 24-12-7
                V2.0.0 on 25-2-21,optimize LowPassFilter(rename,delete template,add constructor,add operator())
                V3.0.0 on 25-4-11,refactor with C++ inheritance
 * */

#ifndef FILTERS_H
#define FILTERS_H

#include "LowPassFilter.h"
#include <cstdlib>
#include <cstring>
#include <numbers>

using namespace std;

/**
 * @brief one-order low pass filter
 */
class LowPassFilter_1_Order final : public LowPassFilter {
public:
    /**
     * @brief constructor
     * @param Ts Low pass filter time constant,unit s
     * @param Fc Low pass filter cut-off frequency,unit Hz
     */
    LowPassFilter_1_Order(const float Ts, const float Fc) :
        Fc(Fc), a(2 * numbers::pi_v<float> * Fc * Ts / (2 * numbers::pi_v<float> * Fc * Ts + 1)) {}

    float Fc; //!< Low pass filter cut-off frequency

    float operator()(const float value) override {
        this->value = a * value + (1 - a) * this->value;
        return this->value;
    }

private:
    float value{0};
    float a{1}; //filter coefficient,default 1(no filter)
};

/**
 * @brief one-order Kalman filter
 */
class KalmanFilter_1_Order final : public LowPassFilter {
public:
    /**
     * @brief constructor
     * @param Q 过程噪声协方差,Q为对模型的信任度,Q越大,滤波后的曲线跟测量曲线跟的越紧密,滤波后噪声越大
     * @param R 观测噪声协方差,R决定稳态噪声,小了初始增益大,但是稳态容易引入噪声;R越大对噪声越不敏感,即滤波后的数据跳动越小,但R越大,kalman滤波输出收敛的越慢
     */
    KalmanFilter_1_Order(const float Q, const float R) : Q(Q), R(R) {}

    float operator()(const float value) override {
        P = P + Q;
        k = P / (P + R);
        x = x + k * (value - x);
        P = (1 - k) * P;
        return x;
    }

private:
    float x{0};
    float P{1};
    float k{0};
    float Q{0};
    float R{0};
};

class MovingAverageFilter final : public LowPassFilter {
public:
    /**
     * @brief constructor
     * @param window_size window size
     */
    explicit MovingAverageFilter(const size_t window_size) : window_size(window_size) {
        values = static_cast<float *>(malloc(window_size * sizeof(float)));
        memset(values, 0, window_size * sizeof(float));
    }

    ~MovingAverageFilter() override { free(values); }

    float operator()(const float value) override {
        sum -= values[index];
        values[index] = value;
        sum += value;
        index = (index + 1) % window_size;
        return sum / window_size;
    }

private:
    size_t index{0};
    size_t window_size{0};
    float sum{0};
    float *values{nullptr};
};


// #define Const_2pi       (6.283185)
// #define Const_TS        (0.001)     //1000us/1KHz
//
// //二阶低通滤波器
// float LPF2(float xin) {
//     float f = 160; //截止频率/Hz
//     float wc = Const_2pi * f;
//     float dampingRatio = 0.707; //阻尼比
//
//     float lpf2_b0 = wc * wc * Const_TS * Const_TS;
//     float lpf2_a0 = 4 + 4 * dampingRatio * wc * Const_TS + lpf2_b0;
//     float lpf2_a1 = -8 + 2 * lpf2_b0;
//     //float lpf2_a2 = 4 - 4*dampingRatio*wc*Const_TS  + lpf2_a0; //原始这里应该有误
//     float lpf2_a2 = lpf2_b0 + 4 - 4 * dampingRatio * wc * Const_TS;
//
//     static float lpf2_yout[3] = {0};
//     static float lpf2_xin[3] = {0};
//
//     lpf2_xin[2] = xin;
//     lpf2_yout[2] = (lpf2_b0 * lpf2_xin[2] + 2 * lpf2_b0 * lpf2_xin[1] + lpf2_b0 * lpf2_xin[0] - lpf2_a1 * lpf2_yout[1] -
//                     lpf2_a2 * lpf2_yout[0]) / lpf2_a0;
//     lpf2_xin[0] = lpf2_xin[1];
//     lpf2_xin[1] = lpf2_xin[2];
//     lpf2_yout[0] = lpf2_yout[1];
//     lpf2_yout[1] = lpf2_yout[2];
//
//     return lpf2_yout[2];
// }

#endif //FILTERS_H
