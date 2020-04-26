= Tessellation & Displacement

== はじめに

本章では「Tessellation」と呼ばれるGPU上でポリゴンを分割する機能と、分割後の頂点群に対してDisplacement mapによって変位させる方法について解説していきます。@<br>{}
本章のサンプルは@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming4}@<br>{}
の「Tessellation」です。

=== 実行環境

 * ComputeShaderが実行できる、シェーダーモデル5.0以上の対応環境
 * Tessellation Shaderのみの使用であれば、シェーダーモデル4.6以上の対応環境
 * 執筆時環境、Unity2018.3.9で動作確認済み

== Tessellationとは

Tessellation（テッセレーション）とは、DirectX、OpenGL、Metal等のレンダリングパイプラインに標準導入されている、GPU上でポリゴンを分割する機能です。@<br>{}
通常、頂点や法線・接線・UV情報等はCPUからGPUに転送されレンダリングパイプラインに流れますが、ハイポリゴンを処理する場合はCPUとGPU間の転送帯域に負荷が生じ、描画速度のボトルネックとなります。@<br>{}
TessellationはGPU上でメッシュを分割する機能を提供しますので、ある程度リダクションされたポリゴンをCPUで処理しつつも、GPUで細分化し、Displacement mapルックアップにより、きめ細やかな変位に戻すといったことが可能になります。@<br>{}
本著では主にUnityでのTessellation機能について解説を行っていきます。

=== Tessellationの各ステージ

Tessellationでは描画パイプライン上に、「Hull Shader」「Tessellation」「Domain Shader」という3つのステージが追加されます。追加されるステージは3つですが、プログラマブルステージは「Hull Shader」と「Domain Shader」の2つのステージのみになります。

//image[sakota/TessellationStage][Tessellation pipeline 出典：Microsoft][scale=0.7]{
//}

//footnote[Tessellation pipeline][https://docs.microsoft.com/en-us/windows/desktop/direct3d11/direct3d-11-advanced-stages-tessellation]


ここで各ステージの詳細を理解し、Hull ShaderとDomain Shaderの実装を行っていくこともTessellationに対しての理解を深める為の1つの手ではありますが、Unityでは非常に便利なWrapperがSurface Shaderに組み込み可能な形で用意されています。@<br>{}
まずはこのSurface Shaderを元に、TessellationとDisplacementを行っていきましょう。


== Surface ShaderとTessellation

UnityがSurface ShaderでサポートしているTessellationについて、コメントで注釈をいれつつ解説を行っていきます。


//emlist[TessellationSurface.Shader]{
Shader "Custom/TessellationDisplacement"
{
    Properties
    {
        _EdgeLength ("Edge length", Range(2,50)) = 15
        _MainTex ("Base (RGB)", 2D) = "white" {}
        _DispTex ("Disp Texture", 2D) = "black" {}
        _NormalMap ("Normalmap", 2D) = "bump" {}
        _Displacement ("Displacement", Range(0, 1.0)) = 0.3
        _Color ("Color", color) = (1,1,1,0)
        _SpecColor ("Spec color", color) = (0.5,0.5,0.5,0.5)
        _Specular ("Specular", Range(0, 1) ) = 0
        _Gloss ("Gloss", Range(0, 1) ) = 0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 300

        CGPROGRAM

        //tessellate:tessEdgeとしてパッチの分割数と手法を定義する関数を指定
        //vertex:dispとして、Displacementを行う関数にdispを指定
        //Wrapper内部ではDomain Shader内でCallされます
        #pragma surface surf BlinnPhong addshadow fullforwardshadows
            vertex:disp tessellate:tessEdge nolightmap
        #pragma target 4.6
        #include "Tessellation.cginc"

        struct appdata
        {
            float4 vertex : POSITION;
            float4 tangent : TANGENT;
            float3 normal : NORMAL;
            float2 texcoord : TEXCOORD0;
        };

        sampler2D _DispTex;
        float _Displacement;
        float _EdgeLength;
        float _Specular;
        float _Gloss;

        //分割数と分割手法を指定する関数です
        //この関数は頂点毎ではなくパッチ毎にCallされます
        //xyzに3頂点で構成されたパッチのエッジの分割数を指定し、
        //wにパッチ内部の分割数を指定して返します
        float4 tessEdge (appdata v0, appdata v1, appdata v2)
        {
            //Tessellation.cginc内に、3種類の分割手法を定義した関数があります

            //カメラからの距離に応じたTessellation
            //UnityDistanceBasedTess

            //Meshのエッジの長さに応じたTessellation
            //UnityEdgeLengthBasedTess

            //UnityEdgeLengthBasedTess関数にカリング機能
            //UnityEdgeLengthBasedTessCull

            return UnityEdgeLengthBasedTessCull(
                v0.vertex, v1.vertex, v2.vertex,
                _EdgeLength, _Displacement * 1.5f
            );
        }

        //Displacement処理関数に指定したdisp関数です。
        //この関数はWrapper内ではTessellator後の
        //Domain Shaderの中で呼ばれています。
        //この関数内でappdataに定義された要素はすべてアクセス可能ですので、
        //Displacementであったりの頂点変調等の処理はここで行います。
        void disp (inout appdata v)
        {
            //ここでは、Displacement mapを使った法線方向への頂点変調を行っています
            float d = tex2Dlod(
                _DispTex,
                float4(v.texcoord.xy,0,0)
            ).r * _Displacement;
            v.vertex.xyz += v.normal * d;
        }

        struct Input
        {
            float2 uv_MainTex;
        };

        sampler2D _MainTex;
        sampler2D _NormalMap;
        fixed4 _Color;

        void surf (Input IN, inout SurfaceOutput o)
        {
            half4 c = tex2D (_MainTex, IN.uv_MainTex) * _Color;
            o.Albedo = c.rgb;
            o.Specular = _Specular;
            o.Gloss = _Gloss;
            o.Normal = UnpackNormal(tex2D(_NormalMap, IN.uv_MainTex));
        }
        ENDCG
    }
    FallBack "Diffuse"
}
//}

上記のShaderでSurface Shaderを利用したDisplacement処理が実現します。非常に安価な実装で大きな恩恵を得ることができます。


== Vertex/Fragment ShaderとTessellation

Vertex/Fragment Shaderに各Tessellationステージを書く場合の実装です。


=== Hull Shader Stage
Hull Shaderはプログラマブル可能なステージで、Vertex Shaderの直後に呼ばれるステージです。ここでは主に「分割方法」と「何分割するか」を定義します。@<br>{}
Hull Shaderは「コントロールポイント関数」と「パッチ定数関数」という2つの関数から構成されており、これらはGPUによって並列に処理されます。コントロールポイントは分割元の制御点のことで、パッチは分割するトポロジーになります。たとえば3角ポリゴン毎にパッチを形成して、そこからTessellatorで分割したい場合は、コントロールポイントが3つで、パッチが1つとなります。@<br>{}
コントロールポイント関数はコントロールポイント毎に動作し、パッチ定数関数はパッチ毎に動作します。


//emlist[Tessellation.Shader]{
#pragma hull hull_shader

//hull shader系の入力として使用する構造体
struct InternalTessInterp_appdata
{
    float4 vertex : INTERNALTESSPOS;
    float4 tangent : TANGENT;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

//パッチ定数関数で定義して返すテッセレーション係数構造体
struct TessellationFactors
{
    float edge[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

// hull constant shader（パッチ定数関数）
TessellationFactors hull_const (InputPatch<InternalTessInterp_appdata, 3> v)
{
    TessellationFactors o;
    float4 tf;

    //Surface shaderでのTessellation時のコメントで解説した分割Utility関数
    tf = UnityEdgeLengthBasedTessCull(
        v[0].vertex, v[1].vertex, v[2].vertex,
        _EdgeLength, _Displacement * 1.5f
    );

    //エッジの分割数をセット
    o.edge[0] = tf.x;
    o.edge[1] = tf.y;
    o.edge[2] = tf.z;
    //中央部の分割数をセット
    o.inside = tf.w;
    return o;
}

// hull shader（制御点関数）

//分割プリミティブタイプ triで3角ポリゴン
[UNITY_domain("tri")]
//integer,fractional_odd,fractional_evenから分割割合を選択
[UNITY_partitioning("fractional_odd")]
//分割後のトポロジー triangle_cwは時計回り3角ポリゴン 反時計回りはtriangle_ccw
[UNITY_outputtopology("triangle_cw")]
//パッチ定数関数名を指定
[UNITY_patchconstantfunc("hull_const")]
//出力コントロールポイント。3角ポリゴンの為3つ出力
[UNITY_outputcontrolpoints(3)]
InternalTessInterp_appdata hull_shader (
    InputPatch<InternalTessInterp_appdata,3> v,
    uint id : SV_OutputControlPointID
)
{
    return v[id];
}

//}


=== Tessellation Stage
ここでHull shaderで返したテッセレーション係数（TessellationFactors構造体）に応じて、パッチに分割処理がかけられます。@<br>{}
Tessellation Stageはプログラマブルではありませんので、関数を記述することはできません。


=== Domain Shader Stage
Domain ShaderはTessellation Stageの処理結果を元に頂点や法線、接線、UV等のポジションの反映を行うプログラマブルなStageです。@<br>{}
SV_DomainLocationというセマンティクスのパラメーターがDomain Shaderに入力されますので、このパラメーターを使って座標を反映させていく形になります。@<br>{}
また、Displacement処理を行いたい場合もDomain Shaderに記述しましょう。
Domain Shaderの後は、Fragment Shaderに処理が流れ最終的な描画処理を行う形になりますが、#pragmaでGeometry Shader関数を指定している場合は、Geometory Shaderに流すことも可能です。


//emlist[Tessellation.Shader]{
#pragma domain domain_shader

struct v2f
{
    UNITY_POSITION(pos);
    float2 uv_MainTex : TEXCOORD0;
    float4 tSpace0 : TEXCOORD1;
    float4 tSpace1 : TEXCOORD2;
    float4 tSpace2 : TEXCOORD3;
};

sampler2D _MainTex;
float4 _MainTex_ST;
sampler2D _DispTex;
float _Displacement

v2f vert_process (appdata v)
{
    v2f o;
    UNITY_INITIALIZE_OUTPUT(v2f,o);
    o.pos = UnityObjectToClipPos(v.vertex);
    o.uv_MainTex.xy = TRANSFORM_TEX(v.texcoord, _MainTex);
    float3 worldPos = mul(unity_ObjectToWorld, v.vertex).xyz;
    float3 worldNormal = UnityObjectToWorldNormal(v.normal);
    fixed3 worldTangent = UnityObjectToWorldDir(v.tangent.xyz);
    fixed tangentSign = v.tangent.w * unity_WorldTransformParams.w;
    fixed3 worldBinormal = cross(worldNormal, worldTangent) * tangentSign;
    o.tSpace0 = float4(
        worldTangent.x, worldBinormal.x, worldNormal.x, worldPos.x
    );
    o.tSpace1 = float4(
        worldTangent.y, worldBinormal.y, worldNormal.y, worldPos.y
    );
    o.tSpace2 = float4(
        worldTangent.z, worldBinormal.z, worldNormal.z, worldPos.z
    );
    return o;
}

void disp (inout appdata v)
{
    float d = tex2Dlod(_DispTex, float4(v.texcoord.xy,0,0)).r * _Displacement;
    v.vertex.xyz -= v.normal * d;
}

// ドメインシェーダー関数
[UNITY_domain("tri")]
v2f domain_shader (
    TessellationFactors tessFactors,
    const OutputPatch<InternalTessInterp_appdata, 3> vi,
    float3 bary : SV_DomainLocation
)
{
    appdata v;
    UNITY_INITIALIZE_OUTPUT(appdata,v);
    //Tessellation stageで計算されたSV_DomainLocationセマンティクスのbaryを元に各座標を設定します
    v.vertex   =
        vi[0].vertex * bary.x +
        vi[1].vertex * bary.y +
        vi[2].vertex * bary.z;
    v.tangent  =
        vi[0].tangent * bary.x +
        vi[1].tangent * bary.y +
        vi[2].tangent * bary.z;
    v.normal   =
        vi[0].normal * bary.x +
        vi[1].normal * bary.y +
        vi[2].normal * bary.z;
    v.texcoord =
        vi[0].texcoord * bary.x +
        vi[1].texcoord * bary.y +
        vi[2].texcoord * bary.z;

    //Displacement処理を行う場合はここが最適になります。
    disp (v);

    //最後にフラグメントシェーダーへ渡す直前の処理を記述します
    v2f o = vert_process (v);
    return o;
}
//}


以上がVertex/Fragment ShaderにTessellationを組み込む際の処理になります。

最後に作例を添付しておきます。この作例は、「Unity Graphics Programming vol.1」に記載している格子法の流体RenderTexture出力をHeight mapとし、Unityに元々入っているPlaneメッシュにTessellationとDisplacement処理を施した物です。@<br>{}
元は頂点数の限られたPlaneメッシュですが、破綻すること無く、メッシュが高い粒度で追従していることが確認できるかと思います。

//image[sakota/TessellationFluid1][流体によるDisplacement][scale=0.9]{
//}



== まとめ

本章では、「Tessellation」についてご紹介しました。@<br>{}
Tessellationはある程度枯れた技術ではありますが、パフォーマンスの最適化からクリエイティブ用途まで、使い勝手がよいものかと思いますので、ぜひ必要な箇所で使っていただければと思います。


== 参考

 * https://docs.unity3d.com/jp/current/Manual/SL-SurfaceShaderTessellation.html
 * https://docs.microsoft.com/en-us/windows/desktop/direct3d11/direct3d-11-advanced-stages-tessellation
