# 解释clamp函数效果

下面是聚光灯内切角与外切角插值判断：

```glsl
  // 聚光灯
  vec3 spotLightDirection = normalize(-light.direction); // 光源方向
  float theta = dot(lightDir, spotLightDirection); // 是cos值，为实际着色点与光源方向的夹角余弦值
  float Phi = cos(light.cutOff); // 是cos值，切光角cos值
  float Phi_outer = cos(light.outerCutOff); // 是cos值，外切光角cos值
  float epsilon = Phi - Phi_outer; // 是cos值，内切光角与外切光角的差值
  float intensity = clamp((theta - Phi_outer) / epsilon, 0.0, 1.0); // 是cos值，内切光角与外切光角的差值

```

假设我们设置：

- 内切光角 = 12.5°，则 Phi = cos(12.5°) ≈ 0.976

- 外切光角 = 17.5°，则 Phi_outer = cos(17.5°) ≈ 0.954

- epsilon = Phi - Phi_outer = 0.976 - 0.954 = 0.022 （这是一个很小的值！）

现在我们来看几个情况：

1. 当光线在内切光角内（比如10°）：
   ```txt
   theta = cos(10°) ≈ 0.985
   (theta - Phi_outer) / epsilon
   = (0.985 - 0.954) / 0.022
   = 0.031 / 0.022
   ≈ 1.41 > 1  // clamp 会把它限制为 1
   ```

2. 当光线在外切光角外（比如20°）：
   ```txt
   theta = cos(20°) ≈ 0.940
   (theta - Phi_outer) / epsilon
   = (0.940 - 0.954) / 0.022
   = -0.014 / 0.022
   ≈ -0.64 < 0  // clamp 会把它限制为 0
   ```

3. 当光线在过渡区域（比如15°）：
   ```txt
   theta = cos(15°) ≈ 0.966
   (theta - Phi_outer) / epsilon
   = (0.966 - 0.954) / 0.022
   = 0.012 / 0.022
   ≈ 0.545  // 在0到1之间，保持原值
   ```

关键点是：

1. epsilon（Phi - Phi_outer）是一个很小的值（因为是两个相近角度的余弦值之差）

1. 正是因为除以这个小值，才能把过渡区域的值"拉伸"到0-1范围内

1. 超出范围的值会被clamp函数限制在0-1之间

这就是为什么这个公式能够有效地创建平滑过渡效果！











