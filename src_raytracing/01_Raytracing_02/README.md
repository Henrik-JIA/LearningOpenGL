## 渐进式渲染公式推导

### 递推公式
$$
S_N = \frac{1}{N}x_N + \frac{N-1}{N}S_{N-1}
$$

### 展开形式
$$
S_N = \frac{1}{N}\sum_{i=1}^{N}x_i = \frac{x_1 + x_2 + \cdots + x_N}{N}
$$

$$
\begin{align*}
S_{k+1} &= \frac{1}{k+1}x_{k+1} + \frac{k}{k+1}S_k \\
&= \frac{1}{k+1}x_{k+1} + \frac{k}{k+1}\left(\frac{1}{k}\sum_{i=1}^{k}x_i\right) \\
&= \frac{1}{k+1}\sum_{i=1}^{k+1}x_i
\end{align*}
$$

$$
S_N = \frac{1}{N}\sum_{i=1}^{N}x_i
$$

### 分步示例

1. **N=1**：
$$
S_1 = x_1
$$

2. **N=2**：
$$
S_2 = \frac{x_1 + x_2}{2}
$$

3. **N=3**：
$$
S_3 = \frac{x_1 + x_2 + x_3}{3}
$$

### 工程实现
当前颜色计算：

```glsl
			// 渐进式渲染的时域累积
			// 当前颜色 = 当前帧颜色 + 历史帧颜色
			// 循环开始LoopNum就加1，所以uniform LoopNum设置时已经是1了。之后每循环一次加1。			
			// 当前颜色 = (1/N) * 新采样颜色 + ((N-1)/N) * 历史颜色
			// 其实就是每帧颜色累加，然后除以帧数N。
			vec3 curColor = (1.0 / float(camera.LoopNum))*vec3(rand(), rand(), rand()) 
							+ (float(camera.LoopNum -1)/float(camera.LoopNum))*hist;
```

