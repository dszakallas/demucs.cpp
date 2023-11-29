#include "encdec.hpp"
#include "layers.hpp"
#include "model.hpp"
#include <Eigen/Core>
#include <Eigen/Dense>
#include <cmath>

// forward declaration to apply a frequency encoder
void demucscpp::apply_freq_encoder(struct demucscpp::demucs_model_4s &model,
                                   int encoder_idx,
                                   const Eigen::Tensor3dXf &x_in,
                                   Eigen::Tensor3dXf &x_out)
{
    Eigen::Tensor3dXf x_shuf = x_in.shuffle(Eigen::array<int, 3>({2, 0, 1}));

    // 2D Convolution operation
    Eigen::Tensor3dXf y;

    switch (encoder_idx) {
        case 0:
            y = demucscpp::conv1d<4, 48, 8, 4, 2, 1>(x_shuf, model.encoder_conv_weight[encoder_idx],
                            model.encoder_conv_bias[encoder_idx]);
            break;
        case 1:
            y = demucscpp::conv1d<48, 96, 8, 4, 2, 1>(x_shuf, model.encoder_conv_weight[encoder_idx],
                            model.encoder_conv_bias[encoder_idx]);
            break;
        case 2:
            y = demucscpp::conv1d<96, 192, 8, 4, 2, 1>(x_shuf, model.encoder_conv_weight[encoder_idx],
                            model.encoder_conv_bias[encoder_idx]);
            break;
        case 3:
            y = demucscpp::conv1d<192, 384, 8, 4, 2, 1>(x_shuf, model.encoder_conv_weight[encoder_idx],
                            model.encoder_conv_bias[encoder_idx]);
            break;
    };

    Eigen::Tensor3dXf y_shuff = y.shuffle(Eigen::array<int, 3>({1, 2, 0}));

    y = demucscpp::gelu(y_shuff);

    // swap first and second dimensions
    // from C,H,W into H,C,W
    y_shuff = y.shuffle(Eigen::array<int, 3>({1, 0, 2}));
    y = y_shuff;
    demucscpp::apply_dconv(model, y, 0, 0, encoder_idx, y.dimension(2));

    // swap back from H,C,W to C,H,W
    // then put W in front to use conv1d function for width=1 conv2d
    Eigen::Tensor3dXf y_shuff_2 = y.shuffle(Eigen::array<int, 3>({2, 1, 0}));

    // need rewrite, norm2, glu
    switch (encoder_idx) {
        case 0:
            y = demucscpp::conv1d<48, 96, 1, 1, 0, 1>(y_shuff_2, model.encoder_rewrite_weight[encoder_idx],
                            model.encoder_rewrite_bias[encoder_idx]);
            break;
        case 1:
            y = demucscpp::conv1d<96, 192, 1, 1, 0, 1>(y_shuff_2, model.encoder_rewrite_weight[encoder_idx],
                            model.encoder_rewrite_bias[encoder_idx]);
            break;
        case 2:
            y = demucscpp::conv1d<192, 384, 1, 1, 0, 1>(y_shuff_2, model.encoder_rewrite_weight[encoder_idx],
                            model.encoder_rewrite_bias[encoder_idx]);
            break;
        case 3:
            y = demucscpp::conv1d<384, 768, 1, 1, 0, 1>(y_shuff_2, model.encoder_rewrite_weight[encoder_idx],
                            model.encoder_rewrite_bias[encoder_idx]);
            break;
    };

    y_shuff_2 = y.shuffle(Eigen::array<int, 3>({1, 2, 0}));

    // copy into x_out
    x_out = demucscpp::glu(y_shuff_2, 0);
}

// forward declaration to apply a time encoder
void demucscpp::apply_time_encoder(struct demucscpp::demucs_model_4s &model,
                                   int tencoder_idx,
                                   const Eigen::Tensor3dXf &xt_in,
                                   Eigen::Tensor3dXf &xt_out)
{
    int crop = demucscpp::TIME_BRANCH_LEN_0;
    // switch case for tencoder_idx
    switch (tencoder_idx)
    {
    case 0:
        break;
    case 1:
        crop = demucscpp::TIME_BRANCH_LEN_1;
        break;
    case 2:
        crop = demucscpp::TIME_BRANCH_LEN_2;
        break;
    case 3:
        crop = demucscpp::TIME_BRANCH_LEN_3;
        break;
    default:
        std::cout << "invalid tencoder_idx" << std::endl;
        break;
    }

    // now implement the forward pass
    // first, apply the convolution
    // Conv1d(2, 48, kernel_size=(8,), stride=(4,), padding=(2,))
    Eigen::Tensor3dXf yt;

    switch (tencoder_idx) {
        case 0:
            yt = demucscpp::conv1d<2, 48, 8, 4, 2, 1>(xt_in, model.tencoder_conv_weight[tencoder_idx],
                            model.tencoder_conv_bias[tencoder_idx]);
            break;
        case 1:
            yt = demucscpp::conv1d<48, 96, 8, 4, 2, 1>(xt_in, model.tencoder_conv_weight[tencoder_idx],
                            model.tencoder_conv_bias[tencoder_idx]);
            break;
        case 2:
            yt = demucscpp::conv1d<96, 192, 8, 4, 2, 1>(xt_in, model.tencoder_conv_weight[tencoder_idx],
                            model.tencoder_conv_bias[tencoder_idx]);
            break;
        case 3:
            yt = demucscpp::conv1d<192, 384, 8, 4, 2, 1>(xt_in, model.tencoder_conv_weight[tencoder_idx],
                            model.tencoder_conv_bias[tencoder_idx]);
            break;
    };

    yt = demucscpp::gelu(yt);

    // now dconv time
    demucscpp::apply_dconv(model, yt, 1, 0, tencoder_idx, crop);

    // end of dconv?

    // need rewrite, norm2, glu
    switch (tencoder_idx) {
        case 0:
            yt = demucscpp::conv1d<48, 96, 1, 1, 0, 1>(yt, model.tencoder_rewrite_weight[tencoder_idx],
                            model.tencoder_rewrite_bias[tencoder_idx]);
            break;
        case 1:
            yt = demucscpp::conv1d<96, 192, 1, 1, 0, 1>(yt, model.tencoder_rewrite_weight[tencoder_idx],
                            model.tencoder_rewrite_bias[tencoder_idx]);
            break;
        case 2:
            yt = demucscpp::conv1d<192, 384, 1, 1, 0, 1>(yt, model.tencoder_rewrite_weight[tencoder_idx],
                            model.tencoder_rewrite_bias[tencoder_idx]);
            break;
        case 3:
            yt = demucscpp::conv1d<384, 768, 1, 1, 0, 1>(yt, model.tencoder_rewrite_weight[tencoder_idx],
                            model.tencoder_rewrite_bias[tencoder_idx]);
            break;
    };

    xt_out = demucscpp::glu(yt, 1);
}

// forward declaration to apply a frequency decoder
void demucscpp::apply_freq_decoder(struct demucscpp::demucs_model_4s &model,
                                   int decoder_idx,
                                   const Eigen::Tensor3dXf &x_in,
                                   Eigen::Tensor3dXf &x_out,
                                   const Eigen::Tensor3dXf &skip)
{
    Eigen::Tensor3dXf y = x_in + skip;

    std::cout << "first conv2d!" << std::endl;

    // need rewrite, norm2, glu
    switch (decoder_idx) {
        case 0:
            y = demucscpp::conv2d<384, 768, 3, 3, 1, 1, 1, 1, 1, 1>(
                y, model.decoder_rewrite_weight[decoder_idx],
                            model.decoder_rewrite_bias[decoder_idx]);
            break;
        case 1:
            y = demucscpp::conv2d<192, 384, 3, 3, 1, 1, 1, 1, 1, 1>(
                y, model.decoder_rewrite_weight[decoder_idx],
                            model.decoder_rewrite_bias[decoder_idx]);
            break;
        case 2:
            y = demucscpp::conv2d<96, 192, 3, 3, 1, 1, 1, 1, 1, 1>(
                y, model.decoder_rewrite_weight[decoder_idx],
                            model.decoder_rewrite_bias[decoder_idx]);
            break;
        case 3:
            y = demucscpp::conv2d<48, 96, 3, 3, 1, 1, 1, 1, 1, 1>(
                y, model.decoder_rewrite_weight[decoder_idx],
                            model.decoder_rewrite_bias[decoder_idx]);
            break;
    };

    y = demucscpp::glu(y, 0);

    // swap first and second dimensions
    // from C,H,W into H,C,W
    Eigen::Tensor3dXf y_shuff = y.shuffle(Eigen::array<int, 3>({1, 0, 2}));
    y = y_shuff;

    // start the DConv
 
    std::cout << "then dconv!" << std::endl;

    demucscpp::apply_dconv(model, y, 0, 1, 4-decoder_idx-1, y.dimension(2));

    // dconv finished

    // swap back from H,C,W to C,H,W
    Eigen::Tensor3dXf y_shuff_2 = y.shuffle(Eigen::array<int, 3>({1, 0, 2}));

    // now time for the transpose convolution
    Eigen::Tensor3dXf y_gemm;

    // 2D Convolution operation
    switch (decoder_idx) {
        case 0:
            y = demucscpp::conv2d_tr_old<384, 192, 8, 1, 4, 1, 0, 0, 1, 1>(
                y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
                model.decoder_conv_tr_bias[decoder_idx]);
            y_gemm = demucscpp::conv2d_tr_gemm<384, 192, 8, 1, 4, 1, 0, 0, 1, 1>(
                y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
                model.decoder_conv_tr_bias[decoder_idx]);
            demucscppdebug::debug_tensor_3dxf(y, "y conv2d-tr-old");
            demucscppdebug::debug_tensor_3dxf(y_gemm, "y conv2d-tr-gemm");
            break;
        case 1:
            y = demucscpp::conv2d_tr<192, 96, 8, 1, 4, 1, 0, 0, 1, 1>(
                y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
                model.decoder_conv_tr_bias[decoder_idx]);
            break;
        case 2:
            y = demucscpp::conv2d_tr<96, 48, 8, 1, 4, 1, 0, 0, 1, 1>(
                y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
                model.decoder_conv_tr_bias[decoder_idx]);
            break;
        case 3:
            y = demucscpp::conv2d_tr<48, 16, 8, 1, 4, 1, 0, 0, 1, 1>(
                y_shuff_2, model.decoder_conv_tr_weight[decoder_idx],
                model.decoder_conv_tr_bias[decoder_idx]);
            break;
    };

    std::cout << "last conv2d_tr!" << std::endl;

    int y_dim1_begin = 2;
    int y_dim1_end = y.dimension(1) - 4;

    // remove 2 elements from begin and end of y along dimension 1 (0, 1, 2)
    Eigen::Tensor3dXf y_cropped_2 =
        y.slice(Eigen::array<Eigen::Index, 3>({0, y_dim1_begin, 0}),
                Eigen::array<Eigen::Index, 3>(
                    {y.dimension(0), y_dim1_end, y.dimension(2)}));

    if (decoder_idx < 3)
    {
        x_out = demucscpp::gelu(y_cropped_2);
    }
    else
    {
        std::cout << "last decoder, no gelu" << std::endl;
        // last, no gelu
        x_out = y_cropped_2;
    }
}

// forward declaration to apply a time decoder
void demucscpp::apply_time_decoder(struct demucscpp::demucs_model_4s &model,
                                   int tdecoder_idx,
                                   const Eigen::Tensor3dXf &xt_in,
                                   Eigen::Tensor3dXf &xt_out,
                                   const Eigen::Tensor3dXf &skip)
{
    int crop = demucscpp::TIME_BRANCH_LEN_3;
    int out_length = demucscpp::TIME_BRANCH_LEN_2;
    // switch case for tdecoder_idx
    switch (tdecoder_idx)
    {
    case 0:
        break;
    case 1:
        crop = demucscpp::TIME_BRANCH_LEN_2;
        out_length = demucscpp::TIME_BRANCH_LEN_1;
        break;
    case 2:
        crop = demucscpp::TIME_BRANCH_LEN_1;
        out_length = demucscpp::TIME_BRANCH_LEN_0;
        break;
    case 3:
        crop = demucscpp::TIME_BRANCH_LEN_0;
        out_length = demucscpp::TIME_BRANCH_LEN_IN;
        break;
    default:
        std::cout << "invalid tdecoder_idx" << std::endl;
        break;
    }

    // need rewrite, norm2, glu
    Eigen::Tensor3dXf yt;
    switch (tdecoder_idx) {
        case 0:
            yt = demucscpp::conv1d<384, 768, 3, 1, 1, 1>(
                xt_in + skip, model.tdecoder_rewrite_weight[tdecoder_idx],
                model.tdecoder_rewrite_bias[tdecoder_idx]);
            break;
        case 1:
            yt = demucscpp::conv1d<192, 384, 3, 1, 1, 1>(
                xt_in + skip, model.tdecoder_rewrite_weight[tdecoder_idx],
                model.tdecoder_rewrite_bias[tdecoder_idx]);
            break;
        case 2:
            yt = demucscpp::conv1d<96, 192, 3, 1, 1, 1>(
                xt_in + skip, model.tdecoder_rewrite_weight[tdecoder_idx],
                model.tdecoder_rewrite_bias[tdecoder_idx]);
            break;
        case 3:
            yt = demucscpp::conv1d<48, 96, 3, 1, 1, 1>(
                xt_in + skip, model.tdecoder_rewrite_weight[tdecoder_idx],
                model.tdecoder_rewrite_bias[tdecoder_idx]);
            break;

    };

    yt = demucscpp::glu(yt, 1);

    // start the DConv
    demucscpp::apply_dconv(model, yt, 1, 1, 4-tdecoder_idx-1, crop);

    // dconv finished

    // next, apply the final transpose convolution
    Eigen::Tensor3dXf yt_tmp;

    switch (tdecoder_idx) {
        case 0:
            yt_tmp = demucscpp::conv1d_tr<384, 192, 8, 4, 0, 1>(
                yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
                model.tdecoder_conv_tr_bias[tdecoder_idx]);
            break;
        case 1:
            yt_tmp = demucscpp::conv1d_tr<192, 96, 8, 4, 0, 1>(
                yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
                model.tdecoder_conv_tr_bias[tdecoder_idx]);
            break;
        case 2:
            yt_tmp = demucscpp::conv1d_tr<96, 48, 8, 4, 0, 1>(
                yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
                model.tdecoder_conv_tr_bias[tdecoder_idx]);
            break;
        case 3:
            yt_tmp = demucscpp::conv1d_tr<48, 8, 8, 4, 0, 1>(
                yt, model.tdecoder_conv_tr_weight[tdecoder_idx],
                model.tdecoder_conv_tr_bias[tdecoder_idx]);
            break;
    };

    yt = yt_tmp;

    // remove padding
    // 2:2+length
    Eigen::Tensor3dXf yt_crop =
        yt.slice(Eigen::array<Eigen::Index, 3>({0, 0, 2}),
                 Eigen::array<Eigen::Index, 3>(
                     {yt.dimension(0), yt.dimension(1), out_length}));

    // gelu activation if not last
    if (tdecoder_idx < 3)
    {
        xt_out = demucscpp::gelu(yt_crop);
    }
    else
    {
        xt_out = yt_crop;
    }
}
