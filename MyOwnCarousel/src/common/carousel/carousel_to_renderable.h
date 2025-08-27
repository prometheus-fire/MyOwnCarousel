#pragma once

#include "..\renderable.h"
#include "carousel.h"



struct game_to_renderable {
	
	/*
		I added a new argument eps which is used to give a predetermined
		offset to the points if needed. This is used for example in the
		to_track function to avoid z-fighting with the terrain.
	*/
	static void ct(float* dst, glm::vec3 src, glm::vec3 eps = glm::vec3(0)) {
		dst[0] = src.x + eps.x;
		dst[1] = src.y + eps.y;
		dst[2] = src.z + eps.z;
	}
	static void to_track(const race & r, renderable& r_t)  {
		
		std::vector<float> buffer_pos; 
		std::vector<float> normals;
		std::vector<float> texcoords;
		buffer_pos.resize(r.t().curbs[0].size() * 2 * 3);

		// offset used to avoid z fighting with terrain
		float eps = 0.;

		terrain ter = r.ter();

		for (unsigned int i = 0; i < r.t().curbs[0].size();++i) {
			glm::vec3 c0 = r.t().curbs[0][i % (r.t().curbs[0].size())];
			glm::vec3 c1 = r.t().curbs[1][i % (r.t().curbs[1].size())];

			// calculate positions
			ct(&buffer_pos[(2 * i  ) * 3], c0);
			ct(&buffer_pos[(2 * i+1) * 3], c1);

			// calculate normals
			glm::vec3 n0, n1;
			n0 = ter.normal((c0+c1)*0.5);
			n1 = n0;
			normals.insert(normals.end(), {n0.x, n0.y, n0.z, n1.x, n1.y, n1.z });

			//calculate texcoords based on distance from start
			/*
			(0,1)__________(1,1)  --- end of road (back to start)
				|    ||    |
			    .	 ..    .
			(0,dist) ||   (1, dist)
				.    ..    .
				|    ||    |
			(0,0)__________(1,0)  --- start of road
			*/
			float dist = float(i) / float(r.t().curbs[0].size()); // a scalar that goes from 0 to 1.
			texcoords.insert(texcoords.end(), { 1, dist, 0, dist });
		}

		/*
			Add the first two curbs again to the end of the arrays
			to close the track properly.
		*/
		buffer_pos.insert(buffer_pos.end(), buffer_pos.begin(), buffer_pos.begin() + 6);
		normals.insert(normals.end(), normals.begin(), normals.begin() + 6);
		texcoords.insert(texcoords.end(), texcoords.begin(), texcoords.begin() + 4);


		r_t.add_vertex_attribute<float>(&buffer_pos[0], static_cast<unsigned int>(buffer_pos.size()), 0, 3); // position
		r_t.add_vertex_attribute<float>(&normals[0],	static_cast<unsigned int>(normals.size()),    1, 3); // normal
		r_t.add_vertex_attribute<float>(&texcoords[0], static_cast<unsigned int>(texcoords.size()),   2, 2); // texcoords
	}

	static void to_stick_object(const std::vector<stick_object>& vec, renderable& r_t) {

		std::vector<float> buffer_pos;
		buffer_pos.resize((vec.size()*2) * 3 );
		for (unsigned int i = 0; i < vec.size();++i) {
			ct(&buffer_pos[(2 * i) * 3], vec[i].pos);
			ct(&buffer_pos[(2 * i+1) * 3], vec[i].pos+glm::vec3(0, vec[i].height,0));
		}

		r_t.add_vertex_attribute<float>(&buffer_pos[0], static_cast<unsigned int>(buffer_pos.size()), 0, 3);
	}

	static void to_tree(const race& r, renderable& r_t) {
		to_stick_object(r.trees(), r_t);
	}
	static void to_lamps(const race& r, renderable& r_t) {
		to_stick_object(r.lamps(), r_t);
	}




	static void to_heightfield(const race& r, renderable& r_hf) {
		std::vector<unsigned int > buffer_id;
		const unsigned int& Z =static_cast<unsigned int>(r.ter().size_pix[1]);
		const unsigned int& X =static_cast<unsigned int>(r.ter().size_pix[0]);

		terrain ter = r.ter();

		std::vector<float>   hf3d; // vertex positions
		std::vector<float>   hfNormals; // vertex normals

		for (unsigned int iz = 0; iz < Z; ++iz)
			for (unsigned int ix = 0; ix < X; ++ix) {
				float x, y, z;
				x = ter.rect_xz[0] + (ix / float(X)) * ter.rect_xz[2];
				y = r.ter().hf(ix, iz);
				z = ter.rect_xz[1] + (iz / float(Z)) * ter.rect_xz[3];

				hf3d.push_back(x);
				hf3d.push_back(y);
				hf3d.push_back(z);

				glm::vec3 curr = glm::vec3(x, y, z);
				glm::vec3 curr_normal = ter.normal(curr);

				hfNormals.push_back(curr_normal.x);
				hfNormals.push_back(curr_normal.y);
				hfNormals.push_back(curr_normal.z);
			}

		for (unsigned int iz = 0; iz < Z-1; ++iz)
			for (unsigned int ix = 0; ix < X-1; ++ix) {
				
				buffer_id.push_back((iz * Z) + ix);
				buffer_id.push_back((iz * Z) + ix + 1);
				buffer_id.push_back((iz + 1) * Z + ix + 1);

				buffer_id.push_back((iz * Z) + ix);
				buffer_id.push_back((iz + 1) * Z + ix + 1);
				buffer_id.push_back((iz + 1) * Z + ix);
			}

		r_hf.add_vertex_attribute<float>(&hf3d[0], X * Z * 3, 0, 3);
		r_hf.add_vertex_attribute<float>(&hfNormals[0], X * Z * 3, 1, 3);
		r_hf.add_indices<unsigned int>(&buffer_id[0], static_cast<unsigned int>(buffer_id.size()), GL_TRIANGLES);
	}

};