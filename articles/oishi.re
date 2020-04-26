
= GPU-Based Cloth Simulation

== はじめに


旗・衣服のような外からの力によって変形する平面状のオブジェクトの形状のシミュレーションは@<strong>{布シミュレーション（Cloth Simulation）}と呼ばれ、CG分野ではアニメーション作成に必須のものとして、多くの研究がなされています。
Unityにもすでに実装されていますが、GPUを活用した並列計算の学習、シミュレーションの性質やパラメータの意味を理解することを目的として、この章では、簡単な布シミュレーションの理論とGPUによる実装を紹介いたします。



//image[gcs_result][布シミュレーション]{
//}



== アルゴリズム解説

=== 質点-バネ系（Mass-Spring System）


バネやゴム、クッションのように、力をかけると変形し、力をかけるのをやめると、元の形状に戻るような物体を@<strong>{弾性体（Elastics Body）}と言います。このような弾性体は、1つの位置や姿勢で表すことができないため、物体を点とその間の接続により表し、それぞれの点の動きで形状全体をシミュレートします。この点は、@<strong>{質点（Mass）}と言い、大きさのない質量の塊であると考えます。また、質点同士の接続はバネ（Spring）の性質を持ちます。各バネの伸び縮みを計算することで弾性体をシミュレートする手法を、質点-バネ系（Mass-Spring System）と言い、2次元状に並んだ質点の集合の運動を計算することによる旗や衣服などのシミュレーションは、布シミュレーション（Cloth Simulation）と呼ばれます。



//image[gcs_mass-spring_system][質点-バネ系]{
//}



=== 布シミュレーションに使用するアルゴリズム

==== バネの力


それぞれのバネは、接続された質点に対して、以下の式に従った力を与えます。



//texequation{
F_{spring} = -k \left( I-I_{0} \right) - b v
//}



ここで、@<m>{I}は、バネの現在の長さ（接続された質点同士の距離）、@<m>{I_{0\}}は、シミュレーション開始時のバネの自然長（バネに何も荷重がかかっていない時の長さ）を表します。@<m>{k}は、バネの硬さを表す定数で、@<m>{v}は質点の速度、@<m>{b}は速度の減衰度合いを決定する定数です。この方程式は、バネは接続された質点同士の距離がバネの初期の自然長に戻ろうとする力が常に働くということを意味します。バネの現在の距離が、バネの自然長と大きく異なっていればそれだけ大きな力が働き、質点の現在の速度に比例して減衰します。



//image[gcs_force_spring][バネの力]{
//}



==== バネの構造


このシミュレーションでは、横・縦方向に、基本構造を作るバネを接続し、斜め方向の極端なズレを防ぐため、対角線状に位置する質点間にもバネを接続します。それぞれ、構造バネ（Structure Spring）、せん断バネ（Shear Spring）と呼び、1つの質点は近隣の12個の質点に対してバネを接続します。



//image[gcs_spring_structure][バネの構造]{
//}
 


==== ベレ法（Verlet Method）による位置計算


このシミュレーションでは、ベレ法というリアルタイムアプリケーションによく用いられる手法で質点の位置の計算を行います。
ベレ法は、ニュートンの運動方程式の数値解法の一つであり、分子動力学において原子の動きなどを計算する時に用いられる手法です。
物体の運動を計算する際、通常は速度から位置を求めますが、ベレ法では、@<strong>{現在の位置}と、@<strong>{前のタイムステップでの位置}から@<strong>{次のタイムステップにおける位置}を求めます。



以下、ベレ法の代数方程式の導出を示します。
@<m>{F}は質点に適用される力、@<m>{m}は質点の質量、@<m>{v}は速度、@<m>{x}は位置、@<m>{t}は時刻、@<m>{\Delta t}はシミュレーションのタイムステップ（シミュレーション１回の計算につき、どれだけの時間を進めるのか）とすると、
質点の運動方程式は、



//texequation{
m\dfrac {d^{2}x\left( t\right) }{dt^{2}}=F
//}



と書けます。
この運動方程式を、次の2つのテイラー展開を用いて、代数方程式にすると、



//texequation{
x\left( t+\Delta t\right) =x\left( t\right) +\Delta t\dfrac {dx\left( t\right) }{dt}+\dfrac {1}{2!}\Delta t^{2}\dfrac {dx^{2}\left( t\right) }{dt^{2}}+\dfrac {1}{3!}\Delta t^{3}\dfrac {dx^{3}\left( t\right) }{dt^{3}}+\ldots
//}



//texequation{
x\left( t-\Delta t\right) =x\left( t\right) -\Delta t\dfrac {dx\left( t\right) }{dt}+\dfrac {1}{2!}\Delta t^{2}\dfrac {dx^{2}\left( t\right) }{dt^{2}}-\dfrac {1}{3!}\Delta t^{3}\dfrac {dx^{3}\left( t\right) }{dt^{3}}+\ldots
//}



となります。この2つのテイラー展開式から、2階の微分項について解き、@<m>{\Delta t}の2次以上の項を十分に小さいものとして無視すると、次のように書けます。



//texequation{
\dfrac {dx^{2}\left( t\right) }{dt^{2}}=\dfrac {x\left( t+\Delta t\right) -2x\left( t\right) +x\left( t-\Delta t\right) }{\Delta t^{2}}@<br>{}
//}



2階の微分項を、運動方程式より質量@<m>{m}と力@<m>{F}で表して整理すると、



//texequation{
x\left( t+\Delta t\right) =2x\left( t\right) -x\left( t-\Delta t\right) +\dfrac {\Delta t^{2}}{m}F\left( t\right)
//}



という代数方程式が得られます。このように1つ後のタイムステップにおける位置を、現在の位置、一つ前のタイムステップにおける位置、質量、力、タイムステップの値から求める式が得られます。



速度については、現在の位置と、時間的に一つ前の位置から求めます。



//texequation{
v\left( t\right) =\dfrac {x\left( t\right) -x\left( t-\Delta t\right) }{\Delta t}
//}



この計算によって求められる速度は、精度が高いとは言えませんが、バネの減衰の計算に用いるのみであるので、問題ではないと言えます。


==== 衝突の計算

===== 布と球の衝突の計算


衝突の処理は、「衝突の検出」と「それに対する反応」との二つのフェーズで行います。



衝突の検出は以下の式で行います。



//texequation{
\left\| x\left( t+\Delta t\right) -c\right\| -r < 0
//}



@<m>{c}と@<m>{r}は、球の@<strong>{中心}と@<strong>{半径}、@<m>{x\left( t+\Delta t\right)}は、ベレ法によって求められた次のタイムステップにおける位置です。
もし衝突が検出されたら、質点を球の表面上に移動させることによって、球と布が衝突しない状態にします。
詳細には、衝突点の表面の法線方向に、球の内部にある質点をずらすことによって行います。
質点の位置の更新は、以下の式に従って行います。



//texequation{
\begin{aligned}
d=\dfrac {x\left( t+\Delta t\right) -c}{\left\| x\left( t+\Delta t\right) -c\right\|}
\\
x^{\prime}\left(t + \Delta t\right) = c + dr
\end{aligned}
//}



@<m>{x^{\prime\}\left(t + \Delta t\right)}は、衝突の後更新された位置です。
@<m>{d}は、質点が深く貫通しないとすると、衝突点における表面への法線の許容できる精度の近似とみなすことができます。



//image[gcs_collision_sphere][衝突の計算]{
//}
 


== サンプルプログラム

=== リポジトリ


サンプルプログラムは、下記リポジトリ内の、
@<strong>{Assets/GPUClothSimulation}フォルダ内にあります。



@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming4,https://github.com/IndieVisualLab/UnityGraphicsProgramming4}


==== 実行条件


このサンプルプログラムでは、ComputeShaderを使用しています。
ComputeShaderの動作環境についてはこちらをご確認下さい。



@<href>{https://docs.unity3d.com/ja/2018.3/Manual/class-ComputeShader.html,https://docs.unity3d.com/ja/2018.3/Manual/class-ComputeShader.html}


== 実装の解説

=== 処理の概略


それぞれのコンポーネントやコードがどのように関連して処理を行うかを示した概略図です。
//image[gcs_calculation_flow][各コンポーネント, コードの構造と処理の流れ]{
//}
 



GPUClothSimulation.csはシミュレーションに使用するデータや処理を管理するC#スプリプトです。このスクリプトは、シミュレーションに使用する位置や法線のデータをRenderTextureの形式で作成し管理します。また、Kernels.computeに記述されたカーネルを呼び出すことによって処理を行います。計算結果の視覚化は、GPUClothRenderer.csスクリプトが行います。このスクリプトによって生成されたMeshオブジェクトは、計算結果である位置データ、法線データが格納されたRenderTextureを参照したClothSurface.shaderの処理によって、ジオメトリを変形して描画されます。


==== GPUClothSimulation.cs


シミュレーションを制御するC#スクリプトです。


//emlist{

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class GPUClothSimulation : MonoBehaviour
{
    [Header("Simulation Parameters")]
    // タイムステップ
    public float   TimeStep = 0.01f;
    // シミュレーションの反復回数
    [Range(1, 16)]
    public int     VerletIterationNum = 4;
    // 布の解像度（横, 縦)
    public Vector2Int ClothResolution = new Vector2Int(128, 128);
    // 布のグリッドの間隔（バネの自然長）
    public float   RestLength = 0.02f;
    // 布の伸縮性を決定する定数
    public float   Stiffness = 10000.0f;
    // 速度の減衰定数
    public float   Damp = 0.996f;
    // 質量
    public float   Mass = 1.0f;
    // 重力
    public Vector3 Gravity = new Vector3(0.0f, -9.81f, 0.0f);

    [Header("References")]
    // 衝突用球体のTransformの参照
    public Transform CollisionSphereTransform;
    [Header("Resources")]
    // シミュレーションを行うカーネル
    public ComputeShader KernelCS;

    // 布シミュレーション 位置データのバッファ
    private RenderTexture[] _posBuff;
    // 布シミュレーション 位置データ（ひとつ前のタイムステップ）のバッファ
    private RenderTexture[] _posPrevBuff;
    // 布シミュレーション 法線データのバッファ
    private RenderTexture _normBuff;

    // 布の長さ (横, 縦)
    private Vector2 _totalClothLength;

    [Header("Debug")]
    // デバッグ用にシミュレーションバッファを表示する
    public bool EnableDebugOnGUI = true;
    // デバッグ表示時のバッファの表示スケール
    private float _debugOnGUIScale = 1.0f;

    // シミュレーションリソースを初期化したか
    public bool IsInit { private set; get; }

    // 位置データのバッファを取得
    public RenderTexture GetPositionBuffer()
    {
        return this.IsInit ? _posBuff[0] : null;
    }
    // 法線データのバッファを取得
    public RenderTexture GetNormalBuffer()
    {
        return this.IsInit ? _normBuff : null;
    }
    // 布の解像度を取得
    public Vector2Int GetClothResolution()
    {
        return ClothResolution;
    }

    // ComputeShaderカーネルのX, Y次元のスレッドの数
    const int numThreadsXY = 32;

    void Start()
    {
        var w = ClothResolution.x;
        var h = ClothResolution.y;
        var format = RenderTextureFormat.ARGBFloat;
        var filter = FilterMode.Point; // テクセル間の補間がなされないように
        // シミュレーション用のデータを格納するRenderTextureを作成
        CreateRenderTexture(ref _posBuff,     w, h, format, filter);
        CreateRenderTexture(ref _posPrevBuff, w, h, format, filter);
        CreateRenderTexture(ref _normBuff,    w, h, format, filter);
        // シミュレーション用のデータをリセット
        ResetBuffer();
        // 初期化済みのフラグを True
        IsInit = true;
    }

    void Update()
    {
        // rキーを押したら, シミュレーション用のデータをリセットする
        if (Input.GetKeyUp("r"))
            ResetBuffer();

        // シミュレーションを行う
        Simulation();
    }

    void OnDestroy()
    {
        // シミュレーション用のデータを格納するRenderTextureを削除
        DestroyRenderTexture(ref _posBuff    );
        DestroyRenderTexture(ref _posPrevBuff);
        DestroyRenderTexture(ref _normBuff   );
    }

    void OnGUI()
    {
        // デバッグ用にシミュレーション用データを格納したRenderTextureを描画
        DrawSimulationBufferOnGUI();
    }

    // シミュレーション用データをリセット
    void ResetBuffer()
    {
        ComputeShader cs = KernelCS;
        // カーネルIDを取得
        int kernelId = cs.FindKernel("CSInit");
        // ComputeShaderカーネルの実行スレッドグループの数を計算
        int groupThreadsX = 
            Mathf.CeilToInt((float)ClothResolution.x / numThreadsXY);
        int groupThreadsY = 
            Mathf.CeilToInt((float)ClothResolution.y / numThreadsXY);
        // 布の長さ（横, 縦）の計算
        _totalClothLength = new Vector2(
            RestLength * ClothResolution.x, 
            RestLength * ClothResolution.y
        );
        // パラメータ, バッファをセット
        cs.SetInts  ("_ClothResolution", 
            new int[2] { ClothResolution.x, ClothResolution.y });
        cs.SetFloats("_TotalClothLength",
            new float[2] { _totalClothLength.x, _totalClothLength.y });
        cs.SetFloat ("_RestLength", RestLength);
        cs.SetTexture(kernelId, "_PositionBufferRW",     _posBuff[0]);
        cs.SetTexture(kernelId, "_PositionPrevBufferRW", _posPrevBuff[0]);
        cs.SetTexture(kernelId, "_NormalBufferRW",       _normBuff);
        // カーネルを実行
        cs.Dispatch(kernelId, groupThreadsX, groupThreadsY, 1);
        // バッファをコピー
        Graphics.Blit(_posBuff[0],     _posBuff[1]);
        Graphics.Blit(_posPrevBuff[0], _posPrevBuff[1]);
    }

    // シミュレーション
    void Simulation()
    {
        ComputeShader cs = KernelCS;
        // CSSimulationの計算1回あたりのタイムステップの値の計算
        float timestep = (float)TimeStep / VerletIterationNum;
        // カーネルIDを取得
        int kernelId = cs.FindKernel("CSSimulation");
        // ComputeShaderカーネルの実行スレッドグループの数を計算
        int groupThreadsX = 
            Mathf.CeilToInt((float)ClothResolution.x / numThreadsXY);
        int groupThreadsY = 
            Mathf.CeilToInt((float)ClothResolution.y / numThreadsXY);

        // パラメータをセット
        cs.SetVector("_Gravity", Gravity);
        cs.SetFloat ("_Stiffness", Stiffness);
        cs.SetFloat ("_Damp", Damp);
        cs.SetFloat ("_InverseMass", (float)1.0f / Mass);
        cs.SetFloat ("_TimeStep", timestep);
        cs.SetFloat ("_RestLength", RestLength);
        cs.SetInts  ("_ClothResolution", 
            new int[2] { ClothResolution.x, ClothResolution.y });

        // 衝突用球のパラメータをセット
        if (CollisionSphereTransform != null)
        {
            Vector3 collisionSpherePos = CollisionSphereTransform.position;
            float collisionSphereRad = 
                CollisionSphereTransform.localScale.x * 0.5f + 0.01f;
            cs.SetBool  ("_EnableCollideSphere", true);
            cs.SetFloats("_CollideSphereParams", 
                new float[4] {
                    collisionSpherePos.x,
                    collisionSpherePos.y,
                    collisionSpherePos.z,
                    collisionSphereRad
                });
        }
        else
            cs.SetBool("_EnableCollideSphere", false);

        for (var i = 0; i < VerletIterationNum; i++)
        {           
            // バッファをセット
            cs.SetTexture(kernelId, "_PositionBufferRO",     _posBuff[0]);
            cs.SetTexture(kernelId, "_PositionPrevBufferRO", _posPrevBuff[0]);
            cs.SetTexture(kernelId, "_PositionBufferRW",     _posBuff[1]);
            cs.SetTexture(kernelId, "_PositionPrevBufferRW", _posPrevBuff[1]);
            cs.SetTexture(kernelId, "_NormalBufferRW",       _normBuff);
            // スレッドを実行
            cs.Dispatch(kernelId, groupThreadsX, groupThreadsY, 1);
            // 読み込み用バッファ, 書き込み用バッファを入れ替え
            SwapBuffer(ref _posBuff[0],     ref _posBuff[1]    );
            SwapBuffer(ref _posPrevBuff[0], ref _posPrevBuff[1]);
        }
    }

    // シミュレーション用のデータを格納するRenderTextureを作成
    void CreateRenderTexture(ref RenderTexture buffer, int w, int h, 
        RenderTextureFormat format, FilterMode filter)
    {
        buffer = new RenderTexture(w, h, 0, format)
        {
            filterMode = filter,
            wrapMode   = TextureWrapMode.Clamp,
            hideFlags  = HideFlags.HideAndDontSave,
            enableRandomWrite = true
        };
        buffer.Create();
    }

    // シミュレーション用のデータを格納するRenderTexture[]を作成
    void CreateRenderTexture(ref RenderTexture[] buffer, int w, int h, 
        RenderTextureFormat format, FilterMode filter)
    {
        // 〜 略 〜
    }

    // シミュレーション用のデータを格納するRenderTextureを削除
    void DestroyRenderTexture(ref RenderTexture buffer)
    {
        // 〜 略 〜
    }

    // シミュレーション用のデータを格納するRenderTexture[]を削除
    void DestroyRenderTexture(ref RenderTexture[] buffer)
    {
        // 〜 略 〜
    }

    // マテリアルを削除
    void DestroyMaterial(ref Material mat)
    {
        // 〜 略 〜
    }

    // バッファを入れ替え
    void SwapBuffer(ref RenderTexture ping, ref RenderTexture pong)
    {
        RenderTexture temp = ping;
        ping = pong;
        pong = temp;
    }

    // デバッグ用に、シミュレーションのためのバッファをOnGUI関数内で描画
    void DrawSimulationBufferOnGUI()
    {
        // 〜 略 〜
    }
}

//}


冒頭では、シミュレーションに必要なパラメータを宣言しています。
また、シミュレーションの結果を保持するものとしてRenderTextureを用います。
このシミュレーションに使用し、また得られるデータは 

 1. 位置
 1. 一つ前のタイムステップにおける位置
 1. 法線



です。


===== InitBuffer関数


InitBuffer関数では、計算に必要なデータを格納するRenderTextureを作成します。
位置と、一つ前のタイムステップにおける位置については、それぞれ、一つ前のタイムステップにおけるデータを使用し、それをもとに計算を行うので、読み込み用と書き込み用に2つ作成します。このように、読み込み用のデータと書き込み用のデータを作成し、効率良くシェーダに計算させる手法を、@<strong>{Ping Pong Buffering}と呼びます。



RenderTextureの作成ですが、formatでは、テクスチャの精度（チャンネル数やそれぞれのチャンネルのbit数）を設定します。一般には低い方が処理は高速になりますが、ARGBHalf（チャンネルあたり16bit）では精度が低く、計算結果が不安定になるので、ARGBFloat（チャンネルあたり32bit）に設定します。また、ComputeShaderで計算結果の書き込みを可能にするため、enableRandomWriteは、trueにします。RenderTextureは、コンストラクタの呼び出しだけではハードウェア上に作成されないので、Create関数を実行し、シェーダで使用できる状態にします。


===== ResetBuffer関数


ResetBuffer関数では、シミュレーションに必要なデータを格納するRenderTextureを初期化します。
カーネルIDの取得、スレッドグループ数の計算、布の長さなど各種パラメータ、計算に使用するRenderTextureのセットをComputeShaderに対して行い、Kernels.compute内に書かれたCSInitカーネルを呼び出して処理を行います。
CSInitカーネルの内容については、次のKernels.computeの詳細の中で説明します。


===== Simulation関数


Simulation関数は、実際の布のシミュレーションを行います。冒頭では、ResetBuffer関数同様、カーネルIDの取得、スレッドグループ数の計算、シミュレーションに使用する各種パラメータやRenderTextureのセットを行います。
一度に大きなタイムステップで計算すると、シミュレーションが不安定になるので、Update()内で、タイムステップを小さな値に細かく分けて、シミュレーションを何回かに分割して安定して計算できるようにしています。反復回数については、@<strong>{VerletIterationNum}で設定します。


==== Kernels.compute


実際のシミュレーションなどの処理を記述したComputeShaderです。



このComputeShaderには、

 1. CSInit
 1. CSSimulation



の二つのカーネルがあります。



カーネルは、それぞれ以下のような処理を行います。


===== CSInit


位置と法線の初期値の計算を行います。
質点の位置については、スレッドのID（2次元）をもとに、X-Y平面上に格子状に配置されるように計算します。


===== CSSimulation


シミュレーションを行います。
CSSimulationカーネルの処理の流れを概略的に示すと下記の図のようになります。



//image[gcs_compute_shader_flow][CSSimulationカーネルにおける計算の流れ]{
//}




以下、コードを示します。


//emlist{

#pragma kernel CSInit
#pragma kernel CSSimulation

#define NUM_THREADS_XY 32 // カーネルのスレッド数

// 位置データ（1つ前のタイムステップ）読み込み用
Texture2D<float4>   _PositionPrevBufferRO;
// 位置データ 読み込み用
Texture2D<float4>   _PositionBufferRO;
// 位置データ（1つ前のタイムステップ）書き込み用
RWTexture2D<float4> _PositionPrevBufferRW;
// 位置データ 書き込み用
RWTexture2D<float4> _PositionBufferRW;
// 法線データ 書き込み用
RWTexture2D<float4> _NormalBufferRW;

int2   _ClothResolution;  // 布の解像度（パーティクル数）（横, 縦）
float2 _TotalClothLength; // 布の総合的な長さ

float  _RestLength;       // バネの自然長

float3 _Gravity;          // 重力
float  _Stiffness;        // 布の伸縮度合いを決定する定数
float  _Damp;             // 布の速度の減衰率
float  _InverseMass;      // 1.0/質量

float  _TimeStep;         // タイムステップの大きさ

bool   _EnableCollideSphere; // 衝突処理を行うかのフラグ
float4 _CollideSphereParams; // 衝突処理用パラメータ（pos.xyz, radius）

// 近傍のパーティクルのIDオフセット（x, y）の配列
static const int2 m_Directions[12] =
{
    int2(-1, -1), //  0
    int2( 0, -1), //  1
    int2( 1, -1), //  2
    int2( 1,  0), //  3
    int2( 1,  1), //  4
    int2( 0,  1), //  5
    int2(-1,  1), //  6
    int2(-1,  0), //  7
    int2(-2, -2), //  8
    int2( 2, -2), //  9
    int2( 2,  2), // 10
    int2(-2,  2)  // 11
};
// 近傍のパーティクルのIDのオフセットを返す
int2 NextNeigh(int n)
{
    return m_Directions[n];
}

// シミュレーション用バッファの初期化を行うカーネル
[numthreads(NUM_THREADS_XY, NUM_THREADS_XY, 1)]
void CSInit(uint3 DTid : SV_DispatchThreadID)
{
    uint2 idx = DTid.xy;

    // 位置
    float3 pos = float3(idx.x * _RestLength, idx.y * _RestLength, 0);
    pos.xy -= _TotalClothLength.xy * 0.5;
    // 法線
    float3 nrm = float3(0, 0, -1);
    // バッファに書き込み
    _PositionPrevBufferRW[idx] = float4(pos.xyz, 1.0);
    _PositionBufferRW[idx]     = float4(pos.xyz, 1.0);
    _NormalBufferRW[idx]       = float4(nrm.xyz, 1.0);
}

// シミュレーションを行うカーネル
[numthreads(NUM_THREADS_XY, NUM_THREADS_XY, 1)]
void CSSimulation(uint2 DTid : SV_DispatchThreadID)
{
    int2 idx = (int2)DTid.xy;
    // 布の解像度（パーティクル数）（横, 縦）
    int2   res = _ClothResolution.xy;
    // 位置を読み込み
    float3 pos = _PositionBufferRO[idx.xy].xyz;
    // 位置（ひとつ前のタイムステップ）の読み込み
    float3 posPrev = _PositionPrevBufferRO[idx.xy].xyz;
    // 位置とひとつ前のタイムステップの位置より, 速度を計算
    float3 vel = (pos - posPrev) / _TimeStep;

    float3 normal   = (float3)0; // 法線
    float3 lastDiff = (float3)0; // 法線計算時に使用する方向ベクトル格納用変数
    float  iters    = 0.0;       // 法線計算時のイテレーション数加算用変数

    // パーティクルにかかる力, 初期値として重力の値を代入
    float3 force = _Gravity.xyz;
    // 1.0 / 質量
    float  invMass = _InverseMass;

    // 布の上辺であれば位置を固定するために計算しない
    if (idx.y == _ClothResolution.y - 1)
        return;

    // 近傍のパーティクル（12個）についての計算を行う
    [unroll]
    for (int k = 0; k < 12; k++)
    {
        // 近傍パーティクルのID（座標）のオフセット
        int2 neighCoord = NextNeigh(k);
        // X軸, 端のパーティクルについては計算しない
        if (((idx.x+neighCoord.x) < 0) || ((idx.x+neighCoord.x) > (res.x-1)))
            continue;
        // Y軸, 端のパーティクルについては計算しない
        if (((idx.y+neighCoord.y) < 0) || ((idx.y+neighCoord.y) > (res.y-1)))
            continue;
        // 近傍のパーティクルのID
        int2   idxNeigh = int2(idx.x + neighCoord.x, idx.y + neighCoord.y);
        // 近傍のパーティクルの位置
        float3 posNeigh = _PositionBufferRO[idxNeigh].xyz;
        // 近傍のパーティクルの位置の差
        float3 posDiff = posNeigh - pos;

        // 法線の計算
        // 基点から近傍のパーティクルへの方向ベクトル
        float3 currDiff = normalize(posDiff);
        if ((iters > 0.0) && (k < 8))
        {
            // 1つ前に調べた近傍パーティクルとの方向ベクトルと
            // 現在のものの角度が鈍角であれば
            float a = dot(currDiff, lastDiff);
            if (a > 0.0) {
                // 外積による直行するベクトルを求めて加算
                normal += cross(lastDiff, currDiff);
            }
        }
        lastDiff = currDiff; // 次の近傍パーティクルとの計算時のために保持

        // 近傍パーティクルとのバネの自然長を計算
        float  restLength = length(neighCoord * _RestLength);
        // バネの力を計算
        force += (currDiff*(length(posDiff)-restLength))*_Stiffness-vel*_Damp;
        // 加算
        if (k < 8) iters += 1.0;
    }
    // 法線ベクトルを計算
    normal = normalize(normal / -(iters - 1.0));

    // 加速度
    float3 acc = (float3)0.0;
    // 運動の法則を適用（加速度の大きさは,力の大きさに比例し質量に反比例する）
    acc = force * invMass;

    // ベレ法による位置計算
    float3 tmp = pos;
    pos = pos * 2.0 - posPrev + acc * (_TimeStep * _TimeStep);
    posPrev = tmp; // ひとつ前のタイムステップの位置

    // 衝突を計算
    if (_EnableCollideSphere)
    {
        float3 center = _CollideSphereParams.xyz; // 中心位置
        float  radius = _CollideSphereParams.w;   // 半径

        if (length(pos - center) < radius)
        {
            // 衝突球の中心から,布のパーティクルの位置への単位ベクトルを計算
            float3 collDir = normalize(pos - center);
            // 衝突球の表面にパーティクルの位置を移動
            pos = center + collDir * radius;
        }
    }

    // 書き込み
    _PositionBufferRW[idx.xy]     = float4(pos.xyz,     1.0);
    _PositionPrevBufferRW[idx.xy] = float4(posPrev.xyz, 1.0);
    _NormalBufferRW[idx.xy]       = float4(normal.xyz,  1.0);
}

//}

==== GPUClothRenderer.cs


このコンポーネントでは、

 1. MeshRendererを取得、または作成
 1. 布のシミュレーションの解像度と一致したMeshオブジェクトを生成
 1. Meshを描画するマテリアルに、シミュレーション結果（位置, 法線）を代入



を行います。



詳細はサンプルコードをご確認ください。


==== ClothSurface.shader


このシェーダでは、シミュレーションによって得られた位置、法線データを頂点シェーダ内で取得し、頂点の書き換えることによって、Meshの形状を変化させます。



詳細はサンプルコードをご確認ください


=== 実行結果


実行をすると、球に対して衝突をする布のような動きをするオブジェクトが確認できます。
シミュレーションに用いる各種パラメータを変更すると、動き方の変化がみられます。



@<strong>{TimeStep}は、Update関数が1回実行されるときに進むシミュレーションの時間です。大きくすると、動きの変化が大きくなりますが、大きすぎる値を設定すると、シミュレーションが不安定になり、値が発散してしまいます。



@<strong>{VerletIterationNum}は、Simulation関数内で実行するCSSimulationカーネルの回数で、同じタイムステップの値でも、この値を大きくするとシミュレーションは安定しやすくなりますが、計算負荷は増大してしまいます。



@<strong>{ClothResolution}は、布の解像度です。大きくすると、しわなどのディティールが多く見られるようになりますが、大きくしすぎるとシミュレーションが不安定になります。ComputeShader上で、スレッドサイズを32で設定しているので32の倍数であることが望ましいです。



@<strong>{RestLength}は、バネの自然長です。バネ同士の距離になりますので、布の長さはClothResolution × RestLengthとなります。



@<strong>{Stiffness}は、バネの硬さです。この値を大きくすると、布の伸縮が小さくなりますが、あまり大きくするとシミュレーションが不安定になります。



@<strong>{Damp}は、バネの動く速度の減衰値です。この値を大きくすると、質点が速く動かなくなるようになり振動しにくくなりますが、変化が小さくなってしまいます。



@<strong>{Mass}は、質点の質量です。この値を大きくすると、布が重たくなったような大きな動きをするようになります。



@<strong>{Gravity}は、布にかかる重力です。布の加速度は、この重力とバネの力が合わさったものとなります。



@<strong>{EnableDebugOnGUI}のチェックを入れると、スクリーンの左上に確認用に、シミュレーションの結果である位置・一つ前のタイムステップにおける位置・法線のデータを格納したテクスチャが描画されるようになっています。



@<strong>{R}キーを押すと、布を初期状態に戻るようにしています。シミュレーションが不安定になり、値が発散してしまったときは初期状態に戻すようにしてください。



//image[gcs_result_debug_on_gui][実行結果（計算結果RenderTextureをスクリーンスペースに描画）]{
//}



==== デバッグとしてスプリングを描画


Assets/GPUClothSimulation/Debug/@<strong>{GPUClothSimulationDebugRender.unity}を開くと、質点と質点間を接続したバネをパーティクルとラインで描画したものが確認できます。



//image[gcs_debug_draw_spring][質点とバネをパーティクルとラインで描画]{
//}



== まとめ


質点-バネ系のシミュレーションで得られる動きは、力の加え方によっては複雑に変化し、面白い形状が作成できます。
この章で紹介した布シミュレーションは非常に単純なものです。球のような単純なものではなく複雑なジオメトリのオブジェクトとの衝突、布同士の衝突、摩擦、布の繊維構造の考慮、大きなタイムステップをとったときのシミュレーションの安定性など、様々な課題を克服するために、多くの研究がなされています。物理エンジンに興味のある方は追究してみてはいかがでしょうか。


== 参考
 * [1] Marco Fratarcangeli, "Game Engine Gems 2, GPGPU Cloth simulation using GLSL, OpenCL, and CUDA", (参照 2019-04-06) - @<href>{http://www.cse.chalmers.se/~marcof/publication/geg2011/,http://www.cse.chalmers.se/~marcof/publication/geg2011/}
 * [2] Wikipedia - Verlet integration, (参照 2019-04-06) -@<href>{https://en.wikipedia.org/wiki/Verlet_integration,https://en.wikipedia.org/wiki/Verlet_integration}
 * [3] 藤澤 誠, CGのための物理シミュレーションの基礎, 株式会社マイナビ, 2013
 * [4] 酒井幸市, WebGLによる物理シミュレーション, 株式会社工学社, 2014
 * [5] 酒井幸市, OpenGLで作る力学アニメーション入門, 森北出版株式会社, 2005

