/**
 * @brief       PID基础控制库(C++)
 * @details
 * @author      Haoqi Liu
 * @date        2025-1-20
 * @version     V1.1.0
 * @note
 * @warning
 * @par         历史版本
 *              V1.0.0创建于2024-9-21,更改自C语言版本PID库
 *              V1.1.0创建于2025-1-20,添加类初始化默认限幅参数,现在可以在初始化时设置限幅
 * */

#ifndef PID_H
#define PID_H
#include <cmath>

class PID {
public:
    enum PID_type {
        position_type, //位置式PID
        delta_type,    //增量式PID
    };

    /**
     * @brief 不允许默认初始化,初始化必须要有PID种类,kp,ki,kd参数
     * */
    PID() = delete;

    /**
     * @brief PID结构体初始化函数
     * @param PID_type          PID类型
     * @param kp,ki,kd          PID三个参数
     * @param sum_error_limit_p 积分限幅上限
     * @param sum_error_limit_n 积分限幅下限
     * @param output_limit_p    输出限幅上限
     * @param output_limit_n    输出限幅下限
     * */
    PID(const PID_type PID_type, const float kp, const float ki, const float kd,
        const float sum_error_limit_p = NAN, const float sum_error_limit_n = NAN,
        const float output_limit_p = NAN, const float output_limit_n = NAN)
        : PID_type(PID_type), kp(kp), ki(ki), kd(kd),
          sum_error_limit_p(sum_error_limit_p), sum_error_limit_n(sum_error_limit_n),
          output_limit_p(output_limit_p), output_limit_n(output_limit_n) {}

    /**
     * @brief 设置PID目标值
     * @param Target 新的PID目标值
     * */
    void SetTarget(const float Target) { target = Target; }

    /**
     * @brief 设置PID限幅
     * @param SumError_limit_p PID积分限幅上限
     * @param SumError_limit_n PID积分限幅下限
     * @param Output_limit_p 输出限幅上限
     * @param Output_limit_n 输出限幅下限
     * @note ${限幅值}无论正负,均将目标值限制在+-${限幅值}中,若${限幅值}为0,则不进行限幅
     * */
    void SetLimit(float SumError_limit_p, float SumError_limit_n, float Output_limit_p, float Output_limit_n);

    /**
     * @brief 设置积分值,用于高自由度的积分限幅
     * @param sum_error PID积分值
     */
    void set_sum_error(const float sum_error) { PID::sum_error = sum_error; }

    /**
     * @brief PID计算
     * @param input PID观测值
     * @return PID计算结果
     * */
    [[nodiscard]] float clac(float input);

    /*设置参数*/
    float target{0};   //目标值
    PID_type PID_type; //PID种类,位置式或增量式
    float kp, ki, kd;  //比例、积分、微分系数
private:
    /*限幅参数*/
    float sum_error_limit_p{NAN}; //积分限幅上限,仅位置式有效
    float sum_error_limit_n{NAN}; //积分限幅下限,仅位置式有效
    float output_limit_p{NAN};    //输出限幅上限
    float output_limit_n{NAN};    //输出限幅下限

    /*中间(运行时)变量*/
    float error{0};     //上一次的偏差值
    float pre_error{0}; //上上一次的偏差值,仅增量式PID使用
    float sum_error{0}; //累计偏差值,仅位置式PID使用
    float output{0};    //PID输出值
};

inline float PID::clac(const float input) {
    const float error_ = target - input;
    if (PID_type == position_type) {
        /***位置式PID公式:u=Kpe(t)+Ki*e(t)的积分+Kd[e(t)-e(t-1)]***/
        sum_error += error_;
        /*积分限幅*/
        if (sum_error_limit_p != NAN)
            if (sum_error >= sum_error_limit_p) sum_error = sum_error_limit_p;
        if (sum_error_limit_n != NAN)
            if (sum_error <= sum_error_limit_n) sum_error = sum_error_limit_n;
        output = kp * error_ +
            ki * sum_error +
            kd * (error_ - error);
        error = error_;
    } else if (PID_type == delta_type) {
        /***增量式PID公式:du=Kp[e(t)-e(t-1)]+Kie(t)+Kd[e(t)-2e(t-1)+e(t-2)]***/
        output += kp * (error_ - error) +
            ki * (error_) +
            kd * (error_ - 2 * error + pre_error);
        pre_error = error;
        error = error_;
    }
    /*输出限幅*/
    if (output_limit_p != NAN)
        if (output >= output_limit_p) output = output_limit_p;
    if (output_limit_n != NAN)
        if (output <= output_limit_n) output = output_limit_n;

    return output;
}

#endif //PID_H
