﻿#ifndef DEFINE_INCLUDED
#define DEFINE_INCLUDED

// 次元数
static constexpr int DIM = 2;

/////////////////////////////////////////////
// 以下は計算手法についての選択肢          //
// 何も指定しなければ標準MPS法が採用される //
/////////////////////////////////////////////

// 過剰接近粒子の補正を導入する
// 　長所：計算が安定する（発散しにくくなる）
// 　短所：運動量が不正確になる
// #define MODIFY_TOO_NEAR

// 圧力勾配項の計算で、圧力を2粒子間の中点で評価する
// 　長所：作用・反作用を満たさない非対称な力がなくなる
// 　短所：なし（不安定化？）
#define PRESSURE_GRADIENT_MIDPOINT

// 圧力の計算に陽解法を用いる場合に指定する
// 　長所：1ステップの計算が陰解法に比べて非常に高速
// 　短所：時間刻みが小さくなる・弱圧縮を許容する
// #define PRESSURE_EXPLICIT

// HS法（高精度生成項）, Khayyer and Gotoh (2009)
//   長所：高精度化
//   短所：計算負荷の微増
#define MPS_HS

// HL法（高精度ラプラシアン）, Khayyer and Gotoh (2010)
//   長所：高精度化
//   短所：なし
#define MPS_HL

// ECS法（誤差修正項）, Khayyer and Gotoh (2011)
//   長所：高精度化
//   短所：計算負荷・メモリ使用量の微増
#define MPS_ECS

#endif
