<script>
	import Counter from './Counter.svelte';
	import welcome from '$lib/images/svelte-welcome.webp';
	import welcome_fallback from '$lib/images/svelte-welcome.png';
	import { json } from '@sveltejs/kit';


	let selected;

	let answer = '';

	let listObj = [];
	let list = [];
	async function get({ params }) {
		const response = await fetch('/api/scan');
		listObj = await response.json();
		list = listObj.scanlist;
	} 
</script>

<svelte:head>
	<title>Home</title>
	<meta name="description" content="Svelte demo app" />
</svelte:head>

<section>
	<h1>
		<span class="welcome">
			<picture>
				<source srcset={welcome} type="image/webp" />
				<img src={welcome_fallback} alt="Welcome" />
			</picture>
		</span>

		to your new<br />SvelteKit app
	</h1>




	<form on:click={get}>
		<select value={selected} on:change={() => (answer = '')}>
			{#each list as item}
				<option value={item}>
					{item}
				</option>
			{/each}
		</select>
	
	</form>


</section>

<style lang="postcss">
	:global(html) {
	  background-color: theme(colors.gray.100);
	}
	section {
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
		flex: 0.6;
	}

	h1 {
		width: 100%;
	}

	.welcome {
		display: block;
		position: relative;
		width: 100%;
		height: 0;
		padding: 0 0 calc(100% * 495 / 2048) 0;
	}

	.welcome img {
		position: absolute;
		width: 100%;
		height: 100%;
		top: 0;
		display: block;
	}
</style>
